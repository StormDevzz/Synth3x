package mc

import (
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"time"
)

type MojangService struct {
	Key       string `json:"key"`
	Name      string `json:"name"`
	Status    string `json:"status"`
}

func MojangStatus() ([]string, error) {
	client := &http.Client{Timeout: 10 * time.Second}
	resp, err := client.Get("https://api.mojang.com/")
	if err != nil {
		return nil, fmt.Errorf("Mojang API unreachable: %w", err)
	}
	resp.Body.Close()

	resp2, err := client.Get("https://status.mojang.com/check")
	if err != nil {
		return nil, fmt.Errorf("status check failed: %w", err)
	}
	defer resp2.Body.Close()
	if resp2.StatusCode != 200 {
		return nil, fmt.Errorf("HTTP %d", resp2.StatusCode)
	}
	body, _ := io.ReadAll(resp2.Body)

	var rawEntries []map[string]string
	if err := json.Unmarshal(body, &rawEntries); err != nil {
		return nil, fmt.Errorf("parse failed: %w", err)
	}

	names := map[string]string{
		"minecraft.net":             "Minecraft Website",
		"session.minecraft.net":     "Session Service",
		"account.mojang.com":        "Mojang Accounts",
		"authserver.mojang.com":     "Auth Server",
		"sessionserver.mojang.com":  "Session Server",
		"api.mojang.com":            "Mojang API",
		"textures.minecraft.net":    "Texture Service",
		"mojang.com":                "Mojang Website",
	}

	var results []string
	for _, entry := range rawEntries {
		for key, status := range entry {
			label := names[key]
			if label == "" {
				label = key
			}
			emoji := "green"
			if status != "green" {
				emoji = "red"
			}
			results = append(results, fmt.Sprintf("%s: %s (%s)", label, emoji, status))
		}
	}
	if len(results) == 0 {
		return []string{"Mojang API reachable, no detailed status"}, nil
	}
	return results, nil
}
