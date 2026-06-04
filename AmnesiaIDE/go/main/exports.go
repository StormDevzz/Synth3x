package main

import "C"
import (
	"bytes"
	"fmt"
	"net"
	"os/exec"
	"strings"
	"time"

	"amnesia_bridge/bridge"
	"amnesia_bridge/github"
	"amnesia_bridge/mc"
)

//export gh_auth_store
func gh_auth_store(token *C.char) *C.char {
	t := C.GoString(token)
	if t == "" {
		return C.CString(bridge.GhResult{Ok: false, Info: "token empty"}.JSON())
	}
	if err := bridge.StoreToken(t); err != nil {
		return C.CString(bridge.GhResult{Ok: false, Err: err.Error()}.JSON())
	}
	return C.CString(bridge.GhResult{Ok: true, Info: "token saved"}.JSON())
}

//export gh_auth_check
func gh_auth_check() *C.char {
	t, err := bridge.ReadToken()
	if err != nil {
		return C.CString(bridge.GhResult{Ok: false, Info: err.Error()}.JSON())
	}
	user, err := github.CheckToken(t)
	if err != nil {
		return C.CString(bridge.GhResult{Ok: true, Err: err.Error(), Info: "token saved but unreachable"}.JSON())
	}
	return C.CString(bridge.GhResult{Ok: true, Info: fmt.Sprintf("authenticated as %s", user)}.JSON())
}

//export gh_auth_clear
func gh_auth_clear() *C.char {
	if err := bridge.ClearToken(); err != nil {
		return C.CString(bridge.GhResult{Ok: false, Err: err.Error()}.JSON())
	}
	return C.CString(bridge.GhResult{Ok: true, Info: "token removed"}.JSON())
}

//export gh_list_repos
func gh_list_repos() *C.char {
	t, err := bridge.ReadToken()
	if err != nil {
		return C.CString(bridge.GhResult{Ok: false, Info: err.Error()}.JSON())
	}
	repos, err := github.ListRepos(t)
	if err != nil {
		return C.CString(bridge.GhResult{Ok: false, Err: err.Error()}.JSON())
	}
	var lines []string
	for _, r := range repos {
		p := ""
		if r.Private {
			p = " (private)"
		}
		lines = append(lines, r.FullName+p)
	}
	return C.CString(bridge.GhResult{
		Ok: true, Out: strings.Join(lines, "\n"), Info: fmt.Sprintf("%d repos", len(repos)),
	}.JSON())
}

//export gh_list_issues
func gh_list_issues(repo *C.char) *C.char {
	t, err := bridge.ReadToken()
	if err != nil {
		return C.CString(bridge.GhResult{Ok: false, Info: err.Error()}.JSON())
	}
	issues, err := github.ListIssues(t, C.GoString(repo))
	if err != nil {
		return C.CString(bridge.GhResult{Ok: false, Err: err.Error()}.JSON())
	}
	var lines []string
	for _, iss := range issues {
		lines = append(lines, fmt.Sprintf("#%d %s", iss.Number, iss.Title))
	}
	return C.CString(bridge.GhResult{
		Ok: true, Out: strings.Join(lines, "\n"), Info: fmt.Sprintf("%d issues", len(issues)),
	}.JSON())
}

//export gh_list_releases
func gh_list_releases(repo *C.char) *C.char {
	t, err := bridge.ReadToken()
	if err != nil {
		return C.CString(bridge.GhResult{Ok: false, Info: err.Error()}.JSON())
	}
	releases, err := github.ListReleases(t, C.GoString(repo))
	if err != nil {
		return C.CString(bridge.GhResult{Ok: false, Err: err.Error()}.JSON())
	}
	var lines []string
	for _, rel := range releases {
		lines = append(lines, fmt.Sprintf("%s (%s)", rel.TagName, rel.Name))
	}
	return C.CString(bridge.GhResult{
		Ok: true, Out: strings.Join(lines, "\n"), Info: fmt.Sprintf("%d releases", len(releases)),
	}.JSON())
}

//export gh_create_release
func gh_create_release(repo, tag, name, body *C.char) *C.char {
	t, err := bridge.ReadToken()
	if err != nil {
		return C.CString(bridge.GhResult{Ok: false, Info: err.Error()}.JSON())
	}
	if err := github.CreateRelease(t, C.GoString(repo), C.GoString(tag), C.GoString(name), C.GoString(body)); err != nil {
		return C.CString(bridge.GhResult{Ok: false, Err: err.Error()}.JSON())
	}
	return C.CString(bridge.GhResult{Ok: true, Info: fmt.Sprintf("release %s created", C.GoString(tag))}.JSON())
}

//export gh_user_info
func gh_user_info() *C.char {
	t, err := bridge.ReadToken()
	if err != nil {
		return C.CString(bridge.GhResult{Ok: false, Info: err.Error()}.JSON())
	}
	user, err := github.GetUser(t)
	if err != nil {
		return C.CString(bridge.GhResult{Ok: false, Err: err.Error()}.JSON())
	}
	return C.CString(bridge.GhResult{
		Ok: true,
		Info: fmt.Sprintf("login: %s\nname: %s\npublic repos: %d", user.Login, user.Name, user.PublicRepos),
	}.JSON())
}

// ---------- network ----------

//export gh_net_check
func gh_net_check() *C.char {
	client := bridge.HTTPClient(5 * time.Second)
	_, err := client.Get("https://github.com")
	if err != nil {
		return C.CString(bridge.GhResult{Ok: false, Err: err.Error(), Info: "no connectivity"}.JSON())
	}
	return C.CString(bridge.GhResult{Ok: true, Info: "internet OK"}.JSON())
}

//export gh_dns_lookup
func gh_dns_lookup(host *C.char) *C.char {
	ips, err := net.LookupHost(C.GoString(host))
	if err != nil {
		return C.CString(bridge.GhResult{Ok: false, Err: err.Error()}.JSON())
	}
	return C.CString(bridge.GhResult{
		Ok: true, Out: strings.Join(ips, "\n"), Info: fmt.Sprintf("%d addresses", len(ips)),
	}.JSON())
}

//export gh_http_get
func gh_http_get(url *C.char) *C.char {
	client := bridge.DefaultClient()
	resp, err := client.Get(C.GoString(url))
	if err != nil {
		return C.CString(bridge.GhResult{Ok: false, Err: err.Error()}.JSON())
	}
	defer resp.Body.Close()
	body := new(bytes.Buffer)
	_, _ = body.ReadFrom(resp.Body)
	return C.CString(bridge.GhResult{
		Ok: true, Out: body.String(), Info: fmt.Sprintf("HTTP %d (%d bytes)", resp.StatusCode, body.Len()),
	}.JSON())
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
		return C.CString(bridge.GhResult{Ok: false, Out: o, Err: err.Error()}.JSON())
	}
	return C.CString(bridge.GhResult{Ok: true, Out: o, Info: "Cloned " + C.GoString(url)}.JSON())
}

//export gh_status
func gh_status(dir *C.char) *C.char {
	out, err := exec.Command("git", "-C", C.GoString(dir), "status", "--porcelain").CombinedOutput()
	o := strings.TrimSpace(string(out))
	if err != nil {
		return C.CString(bridge.GhResult{Ok: false, Out: o, Err: err.Error()}.JSON())
	}
	lines := strings.Split(o, "\n")
	info := "clean"
	if len(lines) > 0 && lines[0] != "" {
		info = strings.TrimSpace(lines[0])
	}
	return C.CString(bridge.GhResult{Ok: true, Out: o, Info: info}.JSON())
}

//export gh_commit_push
func gh_commit_push(dir *C.char, msg *C.char) *C.char {
	d := C.GoString(dir)
	m := C.GoString(msg)
	exec.Command("git", "-C", d, "add", ".").Run()
	out1, _ := exec.Command("git", "-C", d, "commit", "-m", m).CombinedOutput()
	s1 := strings.TrimSpace(string(out1))
	if strings.Contains(s1, "nothing to commit") {
		return C.CString(bridge.GhResult{Ok: true, Out: s1, Info: "Nothing to commit"}.JSON())
	}
	out2, err2 := exec.Command("git", "-C", d, "push").CombinedOutput()
	s2 := strings.TrimSpace(string(out2))
	if err2 != nil {
		return C.CString(bridge.GhResult{Ok: false, Out: s2, Err: err2.Error()}.JSON())
	}
	return C.CString(bridge.GhResult{Ok: true, Out: s1 + "\n" + s2, Info: "Committed & pushed"}.JSON())
}

//export gh_pull
func gh_pull(dir *C.char) *C.char {
	out, err := exec.Command("git", "-C", C.GoString(dir), "pull").CombinedOutput()
	o := strings.TrimSpace(string(out))
	if err != nil {
		return C.CString(bridge.GhResult{Ok: false, Out: o, Err: err.Error()}.JSON())
	}
	return C.CString(bridge.GhResult{Ok: true, Out: o, Info: "Pull ok"}.JSON())
}

// ---------- Minecraft ----------

//export mc_ping_server
func mc_ping_server(address *C.char) *C.char {
	res, err := mc.PingServer(C.GoString(address))
	if err != nil {
		return C.CString(bridge.GhResult{Ok: false, Err: err.Error()}.JSON())
	}
	return C.CString(bridge.GhResult{Ok: true, Out: res, Info: "server pinged"}.JSON())
}

//export mc_player_uuid
func mc_player_uuid(name *C.char) *C.char {
	uuid, err := mc.PlayerUUID(C.GoString(name))
	if err != nil {
		return C.CString(bridge.GhResult{Ok: false, Err: err.Error()}.JSON())
	}
	return C.CString(bridge.GhResult{Ok: true, Out: uuid, Info: "UUID found"}.JSON())
}

//export mc_player_name
func mc_player_name(uuid *C.char) *C.char {
	name, err := mc.PlayerName(C.GoString(uuid))
	if err != nil {
		return C.CString(bridge.GhResult{Ok: false, Err: err.Error()}.JSON())
	}
	return C.CString(bridge.GhResult{Ok: true, Out: name, Info: "name found"}.JSON())
}

//export mc_mojang_status
func mc_mojang_status() *C.char {
	services, err := mc.MojangStatus()
	if err != nil {
		return C.CString(bridge.GhResult{Ok: false, Err: err.Error()}.JSON())
	}
	return C.CString(bridge.GhResult{
		Ok: true, Out: strings.Join(services, "\n"), Info: fmt.Sprintf("%d services", len(services)),
	}.JSON())
}

func main() {}
