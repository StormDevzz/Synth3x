package github

import (
	"fmt"
	"os/exec"
	"strings"
)

func Clone(url, dir string) (string, error) {
	cmd := exec.Command("git", "clone", url)
	if dir != "" {
		cmd.Args = append(cmd.Args, dir)
	}
	out, err := cmd.CombinedOutput()
	o := strings.TrimSpace(string(out))
	if err != nil {
		return o, err
	}
	return o, nil
}

func Status(dir string) (string, string, error) {
	out, err := exec.Command("git", "-C", dir, "status", "--porcelain").CombinedOutput()
	o := strings.TrimSpace(string(out))
	if err != nil {
		return o, "", err
	}
	lines := strings.Split(o, "\n")
	info := "clean"
	if len(lines) > 0 && lines[0] != "" {
		info = strings.TrimSpace(lines[0])
	}
	return o, info, nil
}

func CommitPush(dir, msg string) (string, error) {
	exec.Command("git", "-C", dir, "add", ".").Run()
	out1, _ := exec.Command("git", "-C", dir, "commit", "-m", msg).CombinedOutput()
	s1 := strings.TrimSpace(string(out1))
	if strings.Contains(s1, "nothing to commit") {
		return s1, nil
	}
	out2, err2 := exec.Command("git", "-C", dir, "push").CombinedOutput()
	s2 := strings.TrimSpace(string(out2))
	if err2 != nil {
		return s2, fmt.Errorf("%s: %s", err2.Error(), s2)
	}
	return s1 + "\n" + s2, nil
}

func Pull(dir string) (string, error) {
	out, err := exec.Command("git", "-C", dir, "pull").CombinedOutput()
	o := strings.TrimSpace(string(out))
	if err != nil {
		return o, err
	}
	return o, nil
}
