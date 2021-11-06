package engine

import (
	"context"
	"io/fs"
	"os"
	"path/filepath"
	"runtime"
	"sync"
	"sync/atomic"
	"time"

	. "a1liu.com/dumboss/make/util"
	sem "golang.org/x/sync/semaphore"
)

type ruleKind int8
type targetKind int8

const (
	NotRun int32 = iota
	Running
	FinishedRebuild
	FinishedUnchanged
)

const (
	NoneRuleKind ruleKind = iota
	CompileRuleKind
)

const (
	FileTargetKind targetKind = iota
	PhonyTargetKind
	MarkerTargetKind
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
			cacheResult := CheckCacheTyped(desc.Target, RuleKindToString(desc.Kind))
			if !cacheResult.CacheIsValid {
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
	needsRebuild := atomic.LoadInt32(&rule.state.needsRebuild) != 0
	switch rule.TargetKind {
	case FileTargetKind:
		cacheResult := CheckCacheTyped(desc.Target, RuleKindToString(desc.Kind))
		if cacheResult.FileExists && !needsRebuild {
			if !cacheResult.CacheIsValid {
				atomic.StoreInt32(&parent.needsRebuild, 1)
			}

			atomic.StoreInt32(&rule.state.status, FinishedUnchanged)
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
	eng.threadCounter.Release(1)
}

type CacheResult struct {
	FileExists       bool
	CacheEntryExists bool
	CacheIsValid     bool
}

func CheckCache(filePath string, extras ...string) CacheResult {
	_, callerPath, callerLine, ok := runtime.Caller(1)
	Assert(ok)
	callerRelPath, err := filepath.Rel(ProjectDir, callerPath)
	CheckErr(err)

	passThrough := append([]string{callerRelPath, string(callerLine)}, extras...)
	return CheckCacheTyped(filePath, passThrough...)
}

func CheckCacheTyped(filePath string, extras ...string) CacheResult {
	cachePath := EscapeSourcePath(CacheDir, filePath, extras...)

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
		err = os.MkdirAll(CacheDir, fs.ModeDir|fs.ModePerm)
		CheckErr(err)
		file, err := os.Create(cachePath)
		CheckErr(err)
		file.Close()

		result.CacheEntryExists = false
	} else {
		CheckErr(err)
	}

	if !result.CacheEntryExists || !result.FileExists {
		result.CacheIsValid = false
		return result
	}

	cacheModTime, pathModTime := cacheFileStat.ModTime(), pathStat.ModTime()

	currentTime := time.Now().Local()
	err = os.Chtimes(cachePath, currentTime, currentTime)
	CheckErr(err)

	result.CacheIsValid = pathModTime.Before(cacheModTime)
	return result
}
