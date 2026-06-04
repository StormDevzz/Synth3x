package main

import "C"
import (
	"encoding/json"
	"os/exec"
	"strings"
)

// GhResult mirrors the JSON returned to Rust
type GhResult struct {
	Ok   bool   `json:"ok"`
	Out  string `json:"out"`
	Err  string `json:"err"`
	Info string `json:"info"`
}

func jsonResult(ok bool, out, err, info string) *C.char {
	r, _ := json.Marshal(GhResult{Ok: ok, Out: out, Err: err, Info: info})
	return C.CString(string(r))
}

//export gh_clone
func gh_clone(url *C.char, dir *C.char) *C.char {
	cmd := exec.Command("git", "clone", C.GoString(url))
	d := C.GoString(dir)
	if d != "" {
		cmd.Args = append(cmd.Args, d)
	}
	out, err := cmd.CombinedOutput()
	o := strings.TrimSpace(string(out))
	if err != nil {
		return jsonResult(false, o, err.Error(), "")
	}
	return jsonResult(true, o, "", "Cloned "+C.GoString(url))
}

//export gh_status
func gh_status(dir *C.char) *C.char {
	cmd := exec.Command("git", "-C", C.GoString(dir), "status", "--porcelain")
	out, err := cmd.CombinedOutput()
	o := strings.TrimSpace(string(out))
	if err != nil {
		return jsonResult(false, o, err.Error(), "")
	}
	lines := strings.Split(o, "\n")
	info := "clean"
	if len(lines) > 0 && lines[0] != "" {
		info = strings.TrimSpace(lines[0])
	}
	return jsonResult(true, o, "", info)
}

//export gh_commit_push
func gh_commit_push(dir *C.char, msg *C.char) *C.char {
	d := C.GoString(dir)
	m := C.GoString(msg)
	exec.Command("git", "-C", d, "add", ".").Run()
	out1, _ := exec.Command("git", "-C", d, "commit", "-m", m).CombinedOutput()
	s1 := strings.TrimSpace(string(out1))
	if strings.Contains(s1, "nothing to commit") {
		return jsonResult(true, s1, "", "Nothing to commit")
	}
	out2, err2 := exec.Command("git", "-C", d, "push").CombinedOutput()
	s2 := strings.TrimSpace(string(out2))
	if err2 != nil {
		return jsonResult(false, s2, err2.Error(), "")
	}
	return jsonResult(true, s1+"\n"+s2, "", "Committed & pushed")
}

//export gh_pull
func gh_pull(dir *C.char) *C.char {
	d := C.GoString(dir)
	out, err := exec.Command("git", "-C", d, "pull").CombinedOutput()
	o := strings.TrimSpace(string(out))
	if err != nil {
		return jsonResult(false, o, err.Error(), "")
	}
	return jsonResult(true, o, "", "Pull ok")
}

func main() {}
