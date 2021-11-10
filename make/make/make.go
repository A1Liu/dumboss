package makefiles

import (
	. "a1liu.com/dumboss/make/engine"
	. "a1liu.com/dumboss/make/util"
)

func SerializeMakeruleString(rule Rule) []byte {
	ruleLen := len(rule.RuleDescriptor.Target) + 2
	for _, dep := range rule.Dependencies {
		ruleLen += len(dep.Target) + 1
	}

	out := make([]byte, ruleLen)
	cursor := 0
	for i := 0; i < len(rule.RuleDescriptor.Target); i++ {
		out[cursor] = rule.Target[i]
		cursor += 1
	}

	out[cursor] = ':'
	out[cursor+1] = ' '
	cursor += 2
	for _, dep := range rule.Dependencies {
		for i := 0; i < len(dep.Target); i++ {
			out[cursor] = dep.Target[i]
			cursor += 1
		}

		out[cursor] = ' '
		cursor += 1
	}

	out[cursor-1] = '\n'
	return out
}

type Makerule struct {
	Target []byte
	Deps   [][]byte
}

func SerializeMakerule(rule Makerule) []byte {
	ruleLen := len(rule.Target) + 2
	for _, dep := range rule.Deps {
		ruleLen += len(dep) + 1
	}

	out := make([]byte, ruleLen)
	cursor := 0
	for index, b := range rule.Target {
		out[index] = b
		cursor = index + 1
	}

	out[cursor] = ':'
	out[cursor+1] = ' '
	cursor += 2
	for _, dep := range rule.Deps {
		for _, b := range dep {
			out[cursor] = b
			cursor += 1
		}
		out[cursor] = ' '
		cursor += 1
	}

	out[cursor-1] = '\n'
	return out
}

func ParseMakerule(data []byte) Makerule {
	var rule Makerule
	cursor := 0
	for index, b := range data {
		if b == ':' {
			Assert(data[index+1] == ' ')
			rule.Target = data[0:index]
			cursor = index + 2
			break
		}
	}

	deps := make([][]byte, 10)[:0]
	depBytes := data[cursor:]
	escaped := false
	cursor = -1
	for index, b := range depBytes {
		if b == '\\' {
			escaped = !escaped
			continue
		}

		switch b {
		case '\r':
			fallthrough
		case '\n':
			if cursor >= 0 {
				pathBytes := depBytes[cursor:index]
				deps = append(deps, pathBytes)
				cursor = -1
			}

			if !escaped {
				break
			}

		case ' ':
			if cursor >= 0 {
				pathBytes := depBytes[cursor:index]
				deps = append(deps, pathBytes)
				cursor = -1
			}

		default:
			if cursor == -1 {
				cursor = index
			}
		}

		escaped = false
	}

	rule.Deps = deps
	return rule
}
