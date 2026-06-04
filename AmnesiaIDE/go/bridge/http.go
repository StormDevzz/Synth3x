package bridge

import (
	"net/http"
	"time"
)

func HTTPClient(timeout time.Duration) *http.Client {
	return &http.Client{Timeout: timeout}
}

func DefaultClient() *http.Client {
	return HTTPClient(10 * time.Second)
}
