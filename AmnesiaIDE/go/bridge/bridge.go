package bridge

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"strings"
)

type GhResult struct {
	Ok   bool   `json:"ok"`
	Out  string `json:"out"`
	Err  string `json:"err"`
	Info string `json:"info"`
}

func (r GhResult) JSON() string {
	b, _ := json.Marshal(r)
	return string(b)
}

func TokenPath() string {
	home, _ := os.UserHomeDir()
	return filepath.Join(home, ".amnesia", "ghtoken")
}

func ReadToken() (string, error) {
	data, err := os.ReadFile(TokenPath())
	if err != nil {
		return "", fmt.Errorf("not authenticated")
	}
	t := strings.TrimSpace(string(data))
	if t == "" {
		return "", fmt.Errorf("token empty")
	}
	return t, nil
}

func StoreToken(token string) error {
	if token == "" {
		return fmt.Errorf("token empty")
	}
	dir := filepath.Dir(TokenPath())
	if err := os.MkdirAll(dir, 0700); err != nil {
		return err
	}
	return os.WriteFile(TokenPath(), []byte(token), 0600)
}

func ClearToken() error {
	return os.Remove(TokenPath())
}
