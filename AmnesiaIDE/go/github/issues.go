package github

import (
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"time"
)

type Issue struct {
	Number int    `json:"number"`
	Title  string `json:"title"`
	State  string `json:"state"`
}

func ListIssues(token, repo string) ([]Issue, error) {
	url := fmt.Sprintf("https://api.github.com/repos/%s/issues?per_page=20&state=all", repo)
	req, _ := http.NewRequest("GET", url, nil)
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
	var issues []Issue
	if err := json.Unmarshal(body, &issues); err != nil {
		return nil, err
	}
	return issues, nil
}
