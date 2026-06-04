package main

import (
	"encoding/json"
	"fmt"
	"os"
	"os/exec"
	"strings"
)

type Result struct {
	Ok   bool   `json:"ok"`
	Out  string `json:"out"`
	Err  string `json:"err"`
	Info string `json:"info"`
}

func die(msg string) {
	r, _ := json.Marshal(Result{Ok: false, Err: msg})
	fmt.Println(string(r))
	os.Exit(1)
}

func run(dir, cmd string, args ...string) Result {
	c := exec.Command(cmd, args...)
	c.Dir = dir
	out, err := c.CombinedOutput()
	o := strings.TrimSpace(string(out))
	if err != nil {
		return Result{Ok: false, Out: o, Err: err.Error()}
	}
	return Result{Ok: true, Out: o}
}

func clone(args []string) {
	if len(args) < 2 {
		die("usage: gh clone <url> [dir]")
	}
	url := args[1]
	dir := ""
	if len(args) >= 3 {
		dir = args[2]
	}
	cmd := exec.Command("git", "clone", url)
	if dir != "" {
		cmd.Args = append(cmd.Args, dir)
	}
	out, err := cmd.CombinedOutput()
	if err != nil {
		die(fmt.Sprintf("clone failed: %s\n%s", err, out))
	}
	r, _ := json.Marshal(Result{Ok: true, Out: strings.TrimSpace(string(out)), Info: "Cloned " + url})
	fmt.Println(string(r))
}

func status(args []string) {
	if len(args) < 2 {
		die("usage: gh status <dir>")
	}
	r := run(args[1], "git", "status", "--porcelain")
	if !r.Ok {
		die(r.Err)
	}
	lines := strings.Split(r.Out, "\n")
	info := "clean"
	if len(lines) > 0 && lines[0] != "" {
		info = fmt.Sprintf("%d file(s) changed", len(lines))
	}
	r.Info = info
	b, _ := json.Marshal(r)
	fmt.Println(string(b))
}

func commitPush(args []string) {
	if len(args) < 3 {
		die("usage: gh commit-push <dir> <message>")
	}
	dir := args[1]
	msg := args[2]
	run(dir, "git", "add", ".")
	r := run(dir, "git", "commit", "-m", msg)
	if !r.Ok && strings.Contains(r.Err, "nothing to commit") {
		r = Result{Ok: true, Out: "Nothing to commit"}
	}
	if r.Ok {
		r2 := run(dir, "git", "push")
		if !r2.Ok {
			die(r2.Err)
		}
		r.Info = "Committed & pushed"
	}
	b, _ := json.Marshal(r)
	fmt.Println(string(b))
}

func pull(args []string) {
	if len(args) < 2 {
		die("usage: gh pull <dir>")
	}
	r := run(args[1], "git", "pull")
	if r.Ok {
		r.Info = "Pull ok"
	}
	b, _ := json.Marshal(r)
	fmt.Println(string(b))
}

func main() {
	if len(os.Args) < 2 {
		die("usage: gh <cmd> [args]")
	}
	switch os.Args[1] {
	case "clone":
		clone(os.Args)
	case "status":
		status(os.Args)
	case "commit-push":
		commitPush(os.Args)
	case "pull":
		pull(os.Args)
	default:
		die("unknown command: " + os.Args[1])
	}
}
