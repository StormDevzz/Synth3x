package mc

import (
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"time"
)

type ServerStatus struct {
	Description string `json:"description"`
	Players     int    `json:"players"`
	MaxPlayers  int    `json:"max_players"`
	Version     string `json:"version"`
	Ping        int64  `json:"ping"`
}

func PingServer(address string) (string, error) {
	url := fmt.Sprintf("https://api.mcsrvstat.us/2/%s", address)
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
		IP     string `json:"ip"`
		Port   int    `json:"port"`
		Online bool   `json:"online"`
		Motd   struct {
			Clean []string `json:"clean"`
		} `json:"motd"`
		Players struct {
			Online int `json:"online"`
			Max    int `json:"max"`
		} `json:"players"`
		Version string `json:"version"`
	}
	if err := json.Unmarshal(body, &data); err != nil {
		return "", fmt.Errorf("parse failed: %w", err)
	}
	if !data.Online {
		return fmt.Sprintf("%s:%d - offline", data.IP, data.Port), nil
	}
	desc := ""
	if len(data.Motd.Clean) > 0 {
		desc = data.Motd.Clean[0]
	}
	return fmt.Sprintf("%s:%d\nonline: %d/%d\nversion: %s\ndescription: %s",
		data.IP, data.Port, data.Players.Online, data.Players.Max, data.Version, desc), nil
}
