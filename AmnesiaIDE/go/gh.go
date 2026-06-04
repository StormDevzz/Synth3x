package main

import "C"
import (
	"encoding/json"
	"fmt"
	"io"
	"net"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"time"
)

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

// ---------- auth ----------

func tokenPath() string {
	home, _ := os.UserHomeDir()
	return filepath.Join(home, ".amnesia", "ghtoken")
}

//export gh_auth_store
func gh_auth_store(token *C.char) *C.char {
	t := C.GoString(token)
	if t == "" {
		return jsonResult(false, "", "", "token empty")
	}
	dir := filepath.Dir(tokenPath())
	os.MkdirAll(dir, 0700)
	err := os.WriteFile(tokenPath(), []byte(t), 0600)
	if err != nil {
		return jsonResult(false, "", err.Error(), "")
	}
	return jsonResult(true, "", "", "token saved")
}

//export gh_auth_check
func gh_auth_check() *C.char {
	data, err := os.ReadFile(tokenPath())
	if err != nil {
		return jsonResult(false, "", "", "no token")
	}
	t := strings.TrimSpace(string(data))
	if t == "" {
		return jsonResult(false, "", "", "token empty")
	}
	// validate with GitHub API
	req, _ := http.NewRequest("GET", "https://api.github.com/user", nil)
	req.Header.Set("Authorization", "Bearer "+t)
	req.Header.Set("User-Agent", "AmnesiaIDE")
	client := http.Client{Timeout: 10 * time.Second}
	resp, err := client.Do(req)
	if err != nil {
		return jsonResult(true, "", err.Error(), "token saved but unreachable")
	}
	defer resp.Body.Close()
	if resp.StatusCode == 200 {
		body, _ := io.ReadAll(resp.Body)
		var u struct {
			Login string `json:"login"`
		}
		json.Unmarshal(body, &u)
		return jsonResult(true, "", "", fmt.Sprintf("authenticated as %s", u.Login))
	}
	return jsonResult(false, "", "", fmt.Sprintf("token invalid (HTTP %d)", resp.StatusCode))
}

//export gh_auth_clear
func gh_auth_clear() *C.char {
	os.Remove(tokenPath())
	return jsonResult(true, "", "", "token removed")
}

//export gh_list_repos
func gh_list_repos() *C.char {
	data, err := os.ReadFile(tokenPath())
	if err != nil {
		return jsonResult(false, "", "", "not authenticated")
	}
	t := strings.TrimSpace(string(data))
	req, _ := http.NewRequest("GET", "https://api.github.com/user/repos?per_page=20&sort=updated", nil)
	req.Header.Set("Authorization", "Bearer "+t)
	req.Header.Set("User-Agent", "AmnesiaIDE")
	client := http.Client{Timeout: 10 * time.Second}
	resp, err := client.Do(req)
	if err != nil {
		return jsonResult(false, "", err.Error(), "")
	}
	defer resp.Body.Close()
	body, _ := io.ReadAll(resp.Body)
	var repos []struct {
		FullName string `json:"full_name"`
		Private  bool   `json:"private"`
	}
	json.Unmarshal(body, &repos)
	var lines []string
	for _, r := range repos {
		p := ""
		if r.Private {
			p = " (private)"
		}
		lines = append(lines, r.FullName+p)
	}
	return jsonResult(true, strings.Join(lines, "\n"), "", fmt.Sprintf("%d repos", len(repos)))
}

// ---------- network ----------

//export gh_net_check
func gh_net_check() *C.char {
	timeout := 5 * time.Second
	client := http.Client{Timeout: timeout}
	_, err := client.Get("https://github.com")
	if err != nil {
		return jsonResult(false, "", err.Error(), "no connectivity")
	}
	return jsonResult(true, "", "", "internet OK")
}

//export gh_dns_lookup
func gh_dns_lookup(host *C.char) *C.char {
	h := C.GoString(host)
	ips, err := net.LookupHost(h)
	if err != nil {
		return jsonResult(false, "", err.Error(), "")
	}
	return jsonResult(true, strings.Join(ips, "\n"), "", fmt.Sprintf("%d addresses", len(ips)))
}

//export gh_http_get
func gh_http_get(url *C.char) *C.char {
	u := C.GoString(url)
	client := http.Client{Timeout: 10 * time.Second}
	resp, err := client.Get(u)
	if err != nil {
		return jsonResult(false, "", err.Error(), "")
	}
	defer resp.Body.Close()
	body, _ := io.ReadAll(resp.Body)
	return jsonResult(true, string(body), "", fmt.Sprintf("HTTP %d (%d bytes)", resp.StatusCode, len(body)))
}

// ---------- git ----------

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
