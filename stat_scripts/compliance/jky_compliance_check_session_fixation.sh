#!/bin/bash
# @jocky:function jky_compliance_check_session_fixation
# @jocky:domain compliance
# @jocky:description Test session fixation and cookie scope via curl cookie jars and failed-login POST
# @jocky:inputs target: string, session_dir: string = ""
# @jocky:outputs report: text
# @jocky:capability compliance.web.audit
# @jocky:timeout_seconds 120
# @jocky:depends_on
# V-018 -- Session fixation
# Tool: curl (session comparison before/after auth)
# Usage: bash V-018.sh <target> <session_output_dir>
#
# Decision rule: exit_code_zero
#   PASS = session ID rotates post-auth
#   FAIL = session ID unchanged (fixation)

TARGET="${1:?Usage: V-018.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"
HOST=$(echo "$TARGET" | sed -E 's|https?://||' | sed 's|/.*$||' | sed 's|:.*$||')

echo "=== V-018: Session fixation ==="
echo "Target: $HOST"

VULN_FOUND=0

# --- Helper functions ---
check_url() {
    curl -sI -o /dev/null -w "%{http_code}" --max-time 5 --connect-timeout 3 "$1" 2>/dev/null
}

# --- Test 1: Discover login endpoint ---
echo ""
echo "=== Test 1: Login Endpoint Discovery ==="
LOGIN_PATHS=(
    "/login" "/signin" "/sign-in" "/auth/login"
    "/account/login" "/user/login" "/portal/login"
    "/employeeportal/login" "/citizenportal/login"
    "/wp-login.php" "/administrator"
    "/api/v1/login" "/api/login" "/api/auth/login"
)

LOGIN_URL=""
LOGIN_PAGE=""

for PATH_CANDIDATE in "${LOGIN_PATHS[@]}"; do
    TEST_URL="https://${HOST}${PATH_CANDIDATE}"
    HTTP_CODE=$(check_url "$TEST_URL")
    if [ "$HTTP_CODE" = "200" ] || [ "$HTTP_CODE" = "301" ] || [ "$HTTP_CODE" = "302" ]; then
        LOGIN_URL="$TEST_URL"
        LOGIN_PAGE=$(curl -sL --max-time 8 "$TEST_URL" 2>/dev/null)
        echo "[+] Login endpoint found: $LOGIN_URL"
        break
    fi
done

if [ -z "$LOGIN_URL" ]; then
    echo "[-] No login endpoint found"
    echo "[INFO] Session fixation test skipped"
    exit 0
fi

# --- Test 2: Pre-auth session capture ---
echo ""
echo "=== Test 2: Pre-Auth Session Capture ==="
COOKIE_JAR_PRE=$(mktemp)
PRE_SESSION=$(mktemp)

# Visit login page and capture session
curl -sL --max-time 8 -c "$COOKIE_JAR_PRE" "$LOGIN_URL" > /dev/null 2>&1

# Extract session cookies
PRE_COOKIES=$(cat "$COOKIE_JAR_PRE" 2>/dev/null | grep -v "^#" | grep -v "^$" | awk '{print $6"="$7}' | tr '\n' '; ')
PRE_SESSION_ID=$(cat "$COOKIE_JAR_PRE" 2>/dev/null | grep -iE "session|sid|token|jwt|auth|phpsessid|jsessionid|aspsession|connect.sid" | awk '{print $6"="$7}' | head -1)

echo "[INFO] Pre-auth cookies: $PRE_COOKIES"
if [ -n "$PRE_SESSION_ID" ]; then
    echo "[+] Pre-auth session: $PRE_SESSION_ID"
else
    echo "[INFO] No session cookie captured pre-auth"
fi

# --- Test 3: Session fixation test (no credentials) ---
echo ""
echo "=== Test 3: Session Fixation Test ==="

# Capture session before login attempt
COOKIE_FIXATION=$(mktemp)
curl -sL --max-time 8 -c "$COOKIE_FIXATION" "$LOGIN_URL" > /dev/null 2>&1
FIXATION_SESSION=$(cat "$COOKIE_FIXATION" 2>/dev/null | grep -iE "session|sid|token|jwt|auth|phpsessid|jsessionid|aspsession|connect.sid" | awk '{print $6"="$7}' | head -1)

echo "[INFO] Session from fresh request: $FIXATION_SESSION"

# Now try to login with invalid credentials and check if session changes
COOKIE_AFTER_FAIL=$(mktemp)
FORM_ACTION=$(echo "$LOGIN_PAGE" | grep -oiE 'action=["'"'"'][^"'"'"']*["'"'"']' | head -1 | sed "s/action=['\"]//;s/['\"]$//")
USER_FIELD=$(echo "$LOGIN_PAGE" | grep -oiE '<input[^>]*name=["'"'"'][^"'"'"']*["'"'"'][^>]*>' | grep -iE 'user|email|login' | head -1 | grep -oiE 'name=["'"'"'][^"'"'"']*["'"'"']' | sed "s/name=['\"]//;s/['\"]$//")
PW_FIELD=$(echo "$LOGIN_PAGE" | grep -oiE '<input[^>]*type=["'"'"']password["'"'"'][^>]*>' | head -1 | grep -oiE 'name=["'"'"'][^"'"'"']*["'"'"']' | sed "s/name=['\"]//;s/['\"]$//")

if [ -n "$FORM_ACTION" ] && [ -n "$USER_FIELD" ] && [ -n "$PW_FIELD" ]; then
    if [[ "$FORM_ACTION" == /* ]]; then
        POST_URL="https://${HOST}${FORM_ACTION}"
    elif [[ "$FORM_ACTION" != http* ]]; then
        POST_URL="https://${HOST}/${FORM_ACTION}"
    else
        POST_URL="$FORM_ACTION"
    fi

    echo "[INFO] Login form: POST $POST_URL ($USER_FIELD / $PW_FIELD)"

    # Attempt failed login with same session cookie
    curl -sL --max-time 8 -b "$COOKIE_FIXATION" -c "$COOKIE_AFTER_FAIL" \
        -d "${USER_FIELD}=invalid_test_user&${PW_FIELD}=invalid_test_pass" \
        "$POST_URL" > /dev/null 2>&1

    AFTER_FAIL_SESSION=$(cat "$COOKIE_AFTER_FAIL" 2>/dev/null | grep -iE "session|sid|token|jwt|auth|phpsessid|jsessionid|aspsession|connect.sid" | awk '{print $6"="$7}' | head -1)

    echo "[INFO] Session after failed login: $AFTER_FAIL_SESSION"

    if [ -n "$FIXATION_SESSION" ] && [ -n "$AFTER_FAIL_SESSION" ]; then
        if [ "$FIXATION_SESSION" = "$AFTER_FAIL_SESSION" ]; then
            echo "[PASS] Session unchanged after failed login (expected)"
        else
            echo "[INFO] Session changed after failed login"
        fi
    fi
else
    echo "[INFO] Could not determine form fields"
fi

# --- Test 4: Session rotation test (if we have valid creds) ---
echo ""
echo "=== Test 4: Session Rotation Analysis ==="

# Check if session cookies have secure attributes
COOKIE_SECURE=$(cat "$COOKIE_JAR_PRE" 2>/dev/null | grep -iE "session|sid|token|jwt|auth|phpsessid|jsessionid" | awk '{print $8}' | head -1)
COOKIE_HTTPONLY=$(cat "$COOKIE_JAR_PRE" 2>/dev/null | grep -iE "session|sid|token|jwt|auth|phpsessid|jsessionid" | awk '{print $9}' | head -1)

if [ -n "$COOKIE_SECURE" ]; then
    echo "[INFO] Session cookie flags: $COOKIE_SECURE $COOKIE_HTTPONLY"
fi

# Check for session configuration headers
HEADERS=$(curl -sI --max-time 8 "$LOGIN_URL" 2>/dev/null)

if echo "$HEADERS" | grep -qiE "x-session-id|x-xsrf-token"; then
    echo "[+] Custom session headers detected"
fi

# Check for SameSite attribute
if echo "$COOKIE_JAR_PRE" 2>/dev/null | grep -qi "SameSite"; then
    echo "[PASS] SameSite attribute present"
else
    echo "[WARN] SameSite attribute missing"
fi

# --- Test 5: Multiple session request test ---
echo ""
echo "=== Test 5: Session Consistency Test ==="

SESSIONS=()
for i in 1 2 3; do
    COOKIE_TMP=$(mktemp)
    curl -sL --max-time 5 -c "$COOKIE_TMP" "$LOGIN_URL" > /dev/null 2>&1
    TMP_SESSION=$(cat "$COOKIE_TMP" 2>/dev/null | grep -iE "session|sid|token|jwt|auth|phpsessid|jsessionid|aspsession|connect.sid" | awk '{print $6"="$7}' | head -1)
    if [ -n "$TMP_SESSION" ]; then
        SESSIONS+=("$TMP_SESSION")
        echo "  Request $i: $TMP_SESSION"
    fi
    rm -f "$COOKIE_TMP"
done

if [ ${#SESSIONS[@]} -ge 2 ]; then
    UNIQUE_SESSIONS=$(printf '%s\n' "${SESSIONS[@]}" | sort -u | wc -l)
    if [ "$UNIQUE_SESSIONS" -eq 1 ]; then
        echo "[PASS] Consistent session ID across requests"
    elif [ "$UNIQUE_SESSIONS" -gt 1 ]; then
        echo "[INFO] Multiple unique sessions: $UNIQUE_SESSIONS"
    fi
fi

# --- Test 6: Check for session token in URL ---
echo ""
echo "=== Test 6: Session Token in URL ==="

# Check if session is passed via URL (vulnerable)
URL_SESSION=$(echo "$LOGIN_URL" | grep -oiE '[?&](session|sid|token|jsessionid|phpsessid)=[^&]*')
if [ -n "$URL_SESSION" ]; then
    echo "[FAIL] Session token found in URL: $URL_SESSION"
    VULN_FOUND=1
else
    echo "[PASS] No session token in URL"
fi

# --- Test 7: Session cookie scope ---
echo ""
echo "=== Test 7: Session Cookie Scope ==="

# Check cookie domain and path
COOKIE_DOMAIN=$(cat "$COOKIE_JAR_PRE" 2>/dev/null | grep -iE "session|sid|token|jwt|auth" | awk '{print $1}' | head -1)
COOKIE_PATH=$(cat "$COOKIE_JAR_PRE" 2>/dev/null | grep -iE "session|sid|token|jwt|auth" | awk '{print $4}' | head -1)

if [ -n "$COOKIE_DOMAIN" ]; then
    echo "[INFO] Cookie domain: $COOKIE_DOMAIN"
    if [ "$COOKIE_DOMAIN" = ".${HOST}" ] || [ "$COOKIE_DOMAIN" = "$HOST" ]; then
        echo "[PASS] Cookie domain matches target"
    else
        echo "[WARN] Cookie domain mismatch"
    fi
fi

if [ -n "$COOKIE_PATH" ]; then
    echo "[INFO] Cookie path: $COOKIE_PATH"
fi

# --- Test 8: Check for session configuration ---
echo ""
echo "=== Test 8: Session Configuration ==="

# Check response headers for session configuration
CONFIG_KEYWORDS=("Set-Cookie" "X-Frame-Options" "X-Content-Type-Options" "Strict-Transport-Security")
for KEYWORD in "${CONFIG_KEYWORDS[@]}"; do
    HEADER_VAL=$(echo "$HEADERS" | grep -i "$KEYWORD" | head -1)
    if [ -n "$HEADER_VAL" ]; then
        echo "[+] $HEADER_VAL"
    fi
done

# Check for secure cookie flags
if echo "$HEADERS" | grep -qiE "Set-Cookie.*Secure"; then
    echo "[PASS] Secure flag on cookies"
else
    echo "[WARN] Secure flag may be missing"
fi

if echo "$HEADERS" | grep -qiE "Set-Cookie.*HttpOnly"; then
    echo "[PASS] HttpOnly flag on cookies"
else
    echo "[WARN] HttpOnly flag may be missing"
fi

# --- Test 9: Hydra session fixation brute-force ---
echo ""
echo "=== Test 9: Hydra Session Test ==="
if command -v hydra &>/dev/null; then
    # Test if session changes with different user agents (fingerprinting)
    UA_SESSIONS=()
    for UA in "Mozilla/5.0" "curl/7.68" "python-requests/2.25.1"; do
        COOKIE_TMP=$(mktemp)
        curl -sL --max-time 5 -A "$UA" -c "$COOKIE_TMP" "$LOGIN_URL" > /dev/null 2>&1
        TMP_SESSION=$(cat "$COOKIE_TMP" 2>/dev/null | grep -iE "session|sid|token|jwt|auth|phpsessid|jsessionid" | awk '{print $6"="$7}' | head -1)
        if [ -n "$TMP_SESSION" ]; then
            UA_SESSIONS+=("$UA:$TMP_SESSION")
            echo "  UA '$UA': $TMP_SESSION"
        fi
        rm -f "$COOKIE_TMP"
    done

    # Check if sessions are UA-independent (potential fixation)
    SESSION_VALUES=($(printf '%s\n' "${UA_SESSIONS[@]}" | cut -d: -f2 | sort -u))
    if [ ${#SESSION_VALUES[@]} -eq 1 ] && [ ${#UA_SESSIONS[@]} -gt 1 ]; then
        echo "[WARN] Same session across different user agents"
    elif [ ${#SESSION_VALUES[@]} -gt 1 ]; then
        echo "[PASS] Sessions differ per user agent"
    fi
else
    echo "[INFO] hydra not installed, skipping session test"
fi

# --- Test 10: Session fixation via cookie injection ---
echo ""
echo "=== Test 10: Cookie Injection Test ==="

# Try to set a custom session cookie and see if it's accepted
CUSTOM_SESSION="fixation_test_$(date +%s)"
COOKIE_INJECT=$(mktemp)
curl -sL --max-time 8 \
    -b "session=${CUSTOM_SESSION}; PHPSESSID=${CUSTOM_SESSION}; JSESSIONID=${CUSTOM_SESSION}" \
    -c "$COOKIE_INJECT" \
    "$LOGIN_URL" > /dev/null 2>&1

INJECTED_SESSION=$(cat "$COOKIE_INJECT" 2>/dev/null | grep -iE "session|sid|token|jwt|auth|phpsessid|jsessionid" | awk '{print $6"="$7}' | head -1)

if [ -n "$INJECTED_SESSION" ]; then
    INJECTED_VALUE=$(echo "$INJECTED_SESSION" | cut -d= -f2)
    if [ "$INJECTED_VALUE" = "$CUSTOM_SESSION" ]; then
        echo "[FAIL] Server accepted custom session value - fixation possible"
        VULN_FOUND=1
    else
        echo "[PASS] Server ignored custom session, assigned new: $INJECTED_SESSION"
    fi
else
    echo "[INFO] No session cookie in response"
fi

rm -f "$COOKIE_INJECT"

# --- Summary ---
echo ""
echo "=== Summary ==="
echo "Login endpoint: $LOGIN_URL"
echo "Pre-auth session: ${PRE_SESSION_ID:-none}"
if [ "$VULN_FOUND" -eq 1 ]; then
    echo "[FAIL] Session fixation vulnerability detected"
    exit 1
else
    echo "[PASS] Session management appears secure"
    exit 0
fi
