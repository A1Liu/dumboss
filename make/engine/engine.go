package engine

import (
	"context"
	"sync"
	"sync/atomic"

	. "a1liu.com/dumboss/make/util"
	sem "golang.org/x/sync/semaphore"
)

type ruleKind int

const (
	NotRun            int32 = 0
	Running           int32 = 1
	FinishedRebuild   int32 = 2
	FinishedUnchanged int32 = 3
)

const (
	NoneRuleKind ruleKind = iota
	CompileRuleKind
)

type RuleDescriptor struct {
	Kind    ruleKind
	IsPhony bool
	Target  string
}

type ruleState struct {
	group        sync.WaitGroup
	status       int32
	needsRebuild int32
}

type Rule struct {
	RuleDescriptor
	Dependencies []RuleDescriptor
	state        ruleState
	Run          func(ctx context.Context, target string)
}

type Engine struct {
	rules         map[RuleDescriptor]*Rule
	threadCounter *sem.Weighted
}

func RuleKindToString(kind ruleKind) string {
	switch kind {
	case NoneRuleKind:
		return "None"

	case CompileRuleKind:
		return "Compile"

	default:
		Assert(false)
		return ""
	}
}

func (eng *Engine) AddRule(rule Rule) {
	if eng.rules == nil {
		eng.rules = map[RuleDescriptor]*Rule{}
	}

	_, contained := eng.rules[rule.RuleDescriptor]
	Assert(!contained)

	r := &rule
	r.state.group.Add(len(rule.Dependencies) + 1)
	r.state.status = NotRun

	eng.rules[rule.RuleDescriptor] = r
}

func (eng *Engine) Make(ctx context.Context, kind ruleKind, target string) {
	if eng.threadCounter == nil {
		eng.threadCounter = sem.NewWeighted(8)
	}

	desc := RuleDescriptor{Kind: kind, Target: target}
	rule, ok := eng.rules[desc]
	Assert(ok)

	firstCtx := ruleState{}
	firstCtx.group.Add(1)
	eng.makeJob(ctx, &firstCtx, rule)
}

func (eng *Engine) makeJob(ctx context.Context, parent *ruleState, rule *Rule) {
	defer parent.group.Done()

	if !atomic.CompareAndSwapInt32(&rule.state.status, NotRun, Running) {
		rule.state.group.Wait()

		if atomic.LoadInt32(&rule.state.status) == FinishedRebuild {
			atomic.StoreInt32(&parent.needsRebuild, 1)
		}

		return
	}

	defer rule.state.group.Done()

	for _, desc := range rule.Dependencies {
		child, ok := eng.rules[desc]
		if !ok {
			if !CacheIsValidTyped(desc.Target, RuleKindToString(desc.Kind)) {
				atomic.StoreInt32(&rule.state.needsRebuild, 1)
			}

			continue
		}

		go eng.makeJob(ctx, &rule.state, child)
	}

	for _, desc := range rule.Dependencies {
		child, ok := eng.rules[desc]
		if !ok {
			continue
		}

		child.state.group.Wait()
	}

	desc := rule.RuleDescriptor
	cacheValid := !rule.IsPhony && CacheIsValidTyped(desc.Target, RuleKindToString(desc.Kind))
	if atomic.LoadInt32(&rule.state.needsRebuild) == 0 && cacheValid {
		atomic.StoreInt32(&rule.state.status, FinishedUnchanged)
		return
	}

	atomic.StoreInt32(&rule.state.status, FinishedRebuild)
	atomic.StoreInt32(&parent.needsRebuild, 1)

	eng.threadCounter.Acquire(ctx, 1)
	rule.Run(ctx, rule.RuleDescriptor.Target)
	eng.threadCounter.Release(1)
}
