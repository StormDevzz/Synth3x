package github

import (
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"time"
)

type Repo struct {
	FullName string `json:"full_name"`
	Private  bool   `json:"private"`
}

func ListRepos(token string) ([]Repo, error) {
	req, _ := http.NewRequest("GET", "https://api.github.com/user/repos?per_page=20&sort=updated", nil)
	req.Header.Set("Authorization", "Bearer "+token)
	req.Header.Set("User-Agent", "AmnesiaIDE")
	client := &http.Client{Timeout: 10 * time.Second}
	resp, err := client.Do(req)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()
	if resp.StatusCode != 200 {
		return nil, fmt.Errorf("HTTP %d", resp.StatusCode)
	}
	body, _ := io.ReadAll(resp.Body)
	var repos []Repo
	if err := json.Unmarshal(body, &repos); err != nil {
		return nil, err
	}
	return repos, nil
}
