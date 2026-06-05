package mc

import (
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"time"
)

type PlayerProfile struct {
	ID   string `json:"id"`
	Name string `json:"name"`
}

func PlayerUUID(name string) (string, error) {
	url := fmt.Sprintf("https://api.mojang.com/users/profiles/minecraft/%s", name)
	client := &http.Client{Timeout: 10 * time.Second}
	resp, err := client.Get(url)
	if err != nil {
		return "", fmt.Errorf("request failed: %w", err)
	}
	defer resp.Body.Close()
	if resp.StatusCode == 204 {
		return "", fmt.Errorf("player not found")
	}
	if resp.StatusCode != 200 {
		return "", fmt.Errorf("HTTP %d", resp.StatusCode)
	}
	body, _ := io.ReadAll(resp.Body)
	var profile PlayerProfile
	if err := json.Unmarshal(body, &profile); err != nil {
		return "", fmt.Errorf("parse failed: %w", err)
	}
	return profile.ID, nil
}

func PlayerName(uuid string) (string, error) {
	url := fmt.Sprintf("https://api.mojang.com/user/profile/%s", uuid)
	client := &http.Client{Timeout: 10 * time.Second}
	resp, err := client.Get(url)
	if err != nil {
		return "", fmt.Errorf("request failed: %w", err)
	}
	defer resp.Body.Close()
	if resp.StatusCode != 200 {
		return "", fmt.Errorf("HTTP %d", resp.StatusCode)
	}
	body, _ := io.ReadAll(resp.Body)
	var profile PlayerProfile
	if err := json.Unmarshal(body, &profile); err != nil {
		return "", fmt.Errorf("parse failed: %w", err)
	}
	return profile.Name, nil
}
