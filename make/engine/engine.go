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
	CompileRule ruleKind = iota
)

type RuleDescriptor struct {
	Kind    ruleKind
	IsPhony bool
	Target  string
}

type Rule struct {
	RuleDescriptor
	Dependencies []RuleDescriptor
	alreadyRun   int32
	Run          func(ctx context.Context, target string)
}

type Engine struct {
	rules         map[RuleDescriptor]*Rule
	threadCounter *sem.Weighted
}

func (eng *Engine) AddRule(rule Rule) {
	if eng.rules == nil {
		eng.rules = map[RuleDescriptor]*Rule{}
	}

	_, contained := eng.rules[rule.RuleDescriptor]
	Assert(!contained)

	eng.rules[rule.RuleDescriptor] = &rule
}

func (eng *Engine) Make(ctx context.Context, kind ruleKind, target string) {
	if eng.threadCounter == nil {
		eng.threadCounter = sem.NewWeighted(8)
	}

	desc := RuleDescriptor{Kind: kind, Target: target}
	rule, ok := eng.rules[desc]
	Assert(ok)

	var wg sync.WaitGroup
	wg.Add(1)
	eng.makeJob(ctx, &wg, rule)
}

func (eng *Engine) makeJob(ctx context.Context, parentGroup *sync.WaitGroup, rule *Rule) {
	if !atomic.CompareAndSwapInt32(&rule.alreadyRun, 0, 1) {
		parentGroup.Done()
		return
	}

	var wg sync.WaitGroup
	wg.Add(len(rule.Dependencies))
	for _, desc := range rule.Dependencies {
		rule, ok := eng.rules[desc]
		Assert(ok)

		go eng.makeJob(ctx, &wg, rule)
	}

	wg.Wait()

	eng.threadCounter.Acquire(ctx, 1)
	rule.Run(ctx, rule.RuleDescriptor.Target)
	eng.threadCounter.Release(1)
	parentGroup.Done()
}
