package mc

import (
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"time"
)

func FetchSkin(uuid string) (string, error) {
	url := fmt.Sprintf("https://sessionserver.mojang.com/session/minecraft/profile/%s", uuid)
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
	var data struct {
		ID   string `json:"id"`
		Name string `json:"name"`
		Properties []struct {
			Name  string `json:"name"`
			Value string `json:"value"`
		} `json:"properties"`
	}
	if err := json.Unmarshal(body, &data); err != nil {
		return "", fmt.Errorf("parse failed: %w", err)
	}
	for _, prop := range data.Properties {
		if prop.Name == "textures" {
			return fmt.Sprintf("%s (%s) - textures available", data.Name, data.ID), nil
		}
	}
	return fmt.Sprintf("%s (%s) - no textures property", data.Name, data.ID), nil
}
