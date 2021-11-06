package engine

import (
	"context"
	"os"
	"sync"
	"sync/atomic"
	"time"

	. "a1liu.com/dumboss/make/util"
	sem "golang.org/x/sync/semaphore"
)

type ruleKind int8
type targetKind int8

const (
	NoneRuleKind ruleKind = iota
	CompileRuleKind
)

const (
	FileTargetKind targetKind = iota
	PhonyTargetKind
	MarkerTargetKind
	// MutatorTargetKind // for clang-format
)

const (
	NotRun int32 = iota
	Running
	FinishedRebuild
	FinishedUnchanged
)

type RuleDescriptor struct {
	Kind   ruleKind
	Target string
}

type ruleState struct {
	group        sync.WaitGroup
	status       int32
	needsRebuild int32
}

type Rule struct {
	RuleDescriptor
	TargetKind   targetKind
	Dependencies []RuleDescriptor
	state        ruleState
	Run          func(ctx context.Context, target string)
}

type Engine struct {
	rules         sync.Map
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

func RuleDoNothing(ctx context.Context, target string) {}

func (eng *Engine) getRule(desc RuleDescriptor) *Rule {
	defaultRule := Rule{
		RuleDescriptor: desc,
		TargetKind:     FileTargetKind,
		Dependencies:   []RuleDescriptor{},
		Run:            RuleDoNothing,
	}

	stored, contained := eng.rules.LoadOrStore(desc, &defaultRule)
	rule := stored.(*Rule)
	if !contained {
		rule.state.group.Add(1)
		rule.state.status = NotRun
	}

	return rule
}

func (eng *Engine) AddRule(rule Rule) {
	stored, contained := eng.rules.LoadOrStore(rule.RuleDescriptor, &rule)
	Assert(!contained)

	r := stored.(*Rule)
	r.state.group.Add(len(r.Dependencies) + 1)
	r.state.status = NotRun
}

func (eng *Engine) Make(ctx context.Context, desc RuleDescriptor) {
	if eng.threadCounter == nil {
		eng.threadCounter = sem.NewWeighted(8)
	}

	rule := eng.getRule(desc)
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
		child := eng.getRule(desc)
		go eng.makeJob(ctx, &rule.state, child)
	}

	for _, desc := range rule.Dependencies {
		child := eng.getRule(desc)
		child.state.group.Wait()
	}

	desc := rule.RuleDescriptor
	needsRebuild := atomic.LoadInt32(&rule.state.needsRebuild) != 0
	switch rule.TargetKind {
	case FileTargetKind:
		cacheResult := CheckUpdateCache(desc.Target, RuleKindToString(desc.Kind))
		if cacheResult.FileExists && !needsRebuild {
			finishStatus := FinishedUnchanged
			if !cacheResult.CacheIsValid {
				atomic.StoreInt32(&parent.needsRebuild, 1)
				finishStatus = FinishedRebuild
			}

			atomic.StoreInt32(&rule.state.status, finishStatus)
			return
		}

	case MarkerTargetKind:
		if !needsRebuild {
			atomic.StoreInt32(&rule.state.status, FinishedUnchanged)
			return
		}

	case PhonyTargetKind:
		// Always execute the rule

	default:
		Assert(false)
		return
	}

	atomic.StoreInt32(&rule.state.status, FinishedRebuild)
	atomic.StoreInt32(&parent.needsRebuild, 1)

	eng.threadCounter.Acquire(ctx, 1)
	rule.Run(ctx, rule.RuleDescriptor.Target)
	UpdateCache(desc.Target, RuleKindToString(desc.Kind))
	eng.threadCounter.Release(1)
}

type CacheResult struct {
	FileExists       bool
	CacheEntryExists bool
	CacheIsValid     bool
}

func CheckUpdateCache(filePath string, extras ...string) CacheResult {
	cachePath := EscapeSourcePath(CacheDir, filePath, extras...)
	result := checkCache(filePath, cachePath)
	updateCacheEntry(cachePath)
	return result
}

func CheckCache(filePath string, extras ...string) CacheResult {
	cachePath := EscapeSourcePath(CacheDir, filePath, extras...)
	return checkCache(filePath, cachePath)
}

func UpdateCache(filePath string, extras ...string) {
	cachePath := EscapeSourcePath(CacheDir, filePath, extras...)
	updateCacheEntry(cachePath)
}

func checkCache(filePath, cachePath string) CacheResult {

	result := CacheResult{
		FileExists:       true,
		CacheEntryExists: true,
	}

	pathStat, err := os.Stat(filePath)
	if os.IsNotExist(err) {
		result.FileExists = false
	} else {
		CheckErr(err)
	}

	cacheFileStat, err := os.Stat(cachePath)
	if os.IsNotExist(err) {
		EnsurePath(cachePath)

		result.CacheEntryExists = false
	} else {
		CheckErr(err)
	}

	if !result.CacheEntryExists || !result.FileExists {
		result.CacheIsValid = false
		return result
	}

	cacheModTime, pathModTime := cacheFileStat.ModTime(), pathStat.ModTime()
	result.CacheIsValid = pathModTime.Before(cacheModTime)
	return result
}

func updateCacheEntry(cachePath string) {
	currentTime := time.Now().Local()
	err := os.Chtimes(cachePath, currentTime, currentTime)
	CheckErr(err)
}
