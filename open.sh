#!/usr/bin/env sh
# Opens the Vitmap editor in your default browser.
# A temporary local server runs just long enough for the page to load (needs python3).
cd "$(dirname "$0")" || exit 1

PY=
for candidate in python3 python; do
    # Skip stubs (e.g. the Windows Store alias) that exist but can't run code.
    if command -v "$candidate" >/dev/null 2>&1 && "$candidate" -c 'import http.server' >/dev/null 2>&1; then
        PY=$candidate
        break
    fi
done
if [ -z "$PY" ]; then
    echo "python3 is required to run the local server." >&2
    exit 1
fi

exec "$PY" - <<'EOF'
import http.server, socketserver, sys, threading, webbrowser

IDLE_SECONDS = 3            # quit this long after the last request
FIRST_REQUEST_SECONDS = 60  # give up if the browser never shows up

class Handler(http.server.SimpleHTTPRequestHandler):
    extensions_map = {**http.server.SimpleHTTPRequestHandler.extensions_map,
                      ".js": "text/javascript", ".mjs": "text/javascript", ".css": "text/css"}
    def end_headers(self):
        self.send_header("Cache-Control", "no-store")
        super().end_headers()
    def log_message(self, *args):
        pass

class Server(socketserver.ThreadingTCPServer):
    daemon_threads = True  # an idle browser preconnect must not keep us alive

with Server(("127.0.0.1", 0), Handler) as httpd:
    url = f"http://localhost:{httpd.server_address[1]}/"
    print(f"Serving at {url} (closes automatically once the page has loaded)")
    threading.Timer(0.2, webbrowser.open, args=(url,)).start()
    httpd.timeout = FIRST_REQUEST_SECONDS
    served = False
    while True:
        handled = []
        httpd.handle_timeout = lambda: handled.append(False)
        httpd.handle_request()
        if handled:  # timed out with no request
            break
        served = True
        httpd.timeout = IDLE_SECONDS
    print("Page loaded; server closed." if served else "No browser connected; server closed.")
EOF
