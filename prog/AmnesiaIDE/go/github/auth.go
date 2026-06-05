package github

import (
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"time"
)

func CheckToken(token string) (string, error) {
	req, _ := http.NewRequest("GET", "https://api.github.com/user", nil)
	req.Header.Set("Authorization", "Bearer "+token)
	req.Header.Set("User-Agent", "AmnesiaIDE")
	client := &http.Client{Timeout: 10 * time.Second}
	resp, err := client.Do(req)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()
	if resp.StatusCode != 200 {
		return "", fmt.Errorf("HTTP %d", resp.StatusCode)
	}
	body, _ := io.ReadAll(resp.Body)
	var u struct {
		Login string `json:"login"`
	}
	json.Unmarshal(body, &u)
	return u.Login, nil
}
