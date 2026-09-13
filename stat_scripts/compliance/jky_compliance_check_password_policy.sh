#!/bin/bash
# @jocky:function jky_compliance_check_password_policy
# @jocky:domain compliance
# @jocky:description Assess registration password policy via passive curl page and field analysis
# @jocky:inputs target: string, session_dir: string = ""
# @jocky:outputs report: text
# @jocky:capability compliance.web.read
# @jocky:timeout_seconds 300
# @jocky:depends_on
# V-013 -- Weak password policy
# Tool: curl (passive registration page analysis)
# Usage: bash V-013.sh <target> <session_output_dir>
#
# Decision rule: exit_code_zero
#   PASS = strong password policy detected
#   FAIL = weak/no password policy found

TARGET="${1:?Usage: V-013.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"
HOST=$(echo "$TARGET" | sed -E 's|https?://||' | sed 's|/.*$||' | sed 's|:.*$||')

echo "=== V-013: Weak password policy ==="
echo "Target: $HOST"

WEAK_POLICY=0
REG_PAGE=""
REG_URL=""

# --- Helper: check URL and return page content if 200 ---
check_url() {
    local url="$1"
    local code
    code=$(curl -sI -o /dev/null -w "%{http_code}" --max-time 8 --connect-timeout 5 "$url" 2>/dev/null)
    if [ "$code" = "200" ] || [ "$code" = "301" ] || [ "$code" = "302" ]; then
        curl -sL --max-time 10 "$url" 2>/dev/null
        return 0
    fi
    return 1
}

# --- Test 1: Discover registration page via multiple methods ---
echo ""
echo "=== Test 1: Registration Page Discovery ==="

# Method 1a: Common registration paths
REG_PATHS=(
    "/register" "/signup" "/sign-up" "/sign_up" "/create-account"
    "/create_account" "/new-user" "/new_user" "/join" "/enroll"
    "/auth/register" "/auth/signup" "/auth/create"
    "/account/register" "/account/signup" "/account/create"
    "/user/register" "/user/signup" "/user/create"
    "/members/register" "/members/signup"
    "/client/register" "/client/signup"
    "/portal/register" "/portal/signup"
    "/app/register" "/app/signup"
    "/web/register" "/web/signup"
    "/Registration.aspx" "/Register.aspx"
    "/registration" "/Registration"
)

echo "[*] Trying common registration paths..."
for PATH_CANDIDATE in "${REG_PATHS[@]}"; do
    TEST_URL="https://${HOST}${PATH_CANDIDATE}"
    if REG_PAGE=$(check_url "$TEST_URL"); then
        REG_URL="$TEST_URL"
        echo "[+] Registration page found: $REG_URL"
        break
    fi
done

# Method 1b: Check robots.txt and sitemap for registration URLs
if [ -z "$REG_PAGE" ]; then
    echo "[*] Checking robots.txt and sitemap.xml..."
    ROBOTS=$(curl -sL --max-time 8 "https://${HOST}/robots.txt" 2>/dev/null)
    SITEMAP_URLS=$(echo "$ROBOTS" | grep -oiE 'Sitemap:.*' | awk '{print $2}')

    # Check robots.txt for register/signup paths
    REG_IN_ROBOTS=$(echo "$ROBOTS" | grep -iE 'register|signup|sign-up|create.account' | head -3)
    if [ -n "$REG_IN_ROBOTS" ]; then
        echo "[+] Registration paths found in robots.txt:"
        echo "$REG_IN_ROBOTS" | sed 's/^/  /'
        # Try the first path found
        ROBOT_PATH=$(echo "$REG_IN_ROBOTS" | grep -oiE '/[a-zA-Z0-9/_-]*' | head -1)
        if [ -n "$ROBOT_PATH" ]; then
            TEST_URL="https://${HOST}${ROBOT_PATH}"
            if REG_PAGE=$(check_url "$TEST_URL"); then
                REG_URL="$TEST_URL"
                echo "[+] Registration page found via robots.txt: $REG_URL"
            fi
        fi
    fi

    # Check sitemap.xml
    if [ -z "$REG_PAGE" ]; then
        for SM_URL in "https://${HOST}/sitemap.xml" $SITEMAP_URLS; do
            SM_CONTENT=$(curl -sL --max-time 8 "$SM_URL" 2>/dev/null)
            SM_REG=$(echo "$SM_CONTENT" | grep -oiE 'https?://[^<]*register[^<]*|https?://[^<]*signup[^<]*|https?://[^<]*sign-up[^<]*' | head -3)
            if [ -n "$SM_REG" ]; then
                echo "[+] Registration URLs found in sitemap:"
                echo "$SM_REG" | sed 's/^/  /'
                SM_FIRST=$(echo "$SM_REG" | head -1)
                if REG_PAGE=$(check_url "$SM_FIRST"); then
                    REG_URL="$SM_FIRST"
                    echo "[+] Registration page found via sitemap: $REG_URL"
                    break
                fi
            fi
        done
    fi
fi

# Method 1c: Scan main page for registration/signup links
if [ -z "$REG_PAGE" ]; then
    echo "[*] Scanning main page for registration links..."
    MAIN_PAGE=$(curl -sL --max-time 10 "https://${HOST}/" 2>/dev/null)
    LINKS=$(echo "$MAIN_PAGE" | grep -oiE 'href=["'"'"'][^"'"'"']*["'"'"']' | grep -iE 'register|signup|sign-up|create.account|new.user|join' | head -5)
    if [ -n "$LINKS" ]; then
        echo "[+] Registration links found on homepage:"
        echo "$LINKS" | sed 's/^/  /'
        # Extract first link URL
        FIRST_LINK=$(echo "$LINKS" | grep -oiE 'href=["'"'"'][^"'"'"']*["'"'"']' | head -1 | sed "s/href=['\"]//;s/['\"]$//")
        if [ -n "$FIRST_LINK" ]; then
            # Handle relative URLs
            if [[ "$FIRST_LINK" == /* ]]; then
                TEST_URL="https://${HOST}${FIRST_LINK}"
            elif [[ "$FIRST_LINK" != http* ]]; then
                TEST_URL="https://${HOST}/${FIRST_LINK}"
            else
                TEST_URL="$FIRST_LINK"
            fi
            if REG_PAGE=$(check_url "$TEST_URL"); then
                REG_URL="$TEST_URL"
                echo "[+] Registration page found via homepage link: $REG_URL"
            fi
        fi
    fi
fi

# Method 1d: Check login page for "create account" / "register" links
if [ -z "$REG_PAGE" ]; then
    echo "[*] Checking login page for registration links..."
    LOGIN_PATHS=("/login" "/signin" "/sign-in" "/auth/login" "/account/login" "/user/login" "/portal/login")
    for LOGIN_PATH in "${LOGIN_PATHS[@]}"; do
        LOGIN_URL="https://${HOST}${LOGIN_PATH}"
        LOGIN_PAGE=$(check_url "$LOGIN_URL" 2>/dev/null) || continue
        REG_LINK=$(echo "$LOGIN_PAGE" | grep -oiE 'href=["'"'"'][^"'"'"']*["'"'"']' | grep -iE 'register|signup|sign-up|create.account|join' | head -1)
        if [ -n "$REG_LINK" ]; then
            echo "[+] Registration link found on login page:"
            echo "  $REG_LINK"
            REG_LINK_URL=$(echo "$REG_LINK" | grep -oiE 'href=["'"'"'][^"'"'"']*["'"'"']' | sed "s/href=['\"]//;s/['\"]$//")
            if [[ "$REG_LINK_URL" == /* ]]; then
                TEST_URL="https://${HOST}${REG_LINK_URL}"
            elif [[ "$REG_LINK_URL" != http* ]]; then
                TEST_URL="https://${HOST}/${REG_LINK_URL}"
            else
                TEST_URL="$REG_LINK_URL"
            fi
            if REG_PAGE=$(check_url "$TEST_URL"); then
                REG_URL="$TEST_URL"
                echo "[+] Registration page found via login page: $REG_URL"
                break
            fi
        fi
    done
fi

# Method 1e: CMS-specific paths
if [ -z "$REG_PAGE" ]; then
    echo "[*] Checking CMS-specific registration paths..."
    CMS_PATHS=(
        "/wp-login.php?action=register"           # WordPress
        "/user/register"                           # Drupal
        "/administrator/"                          # Joomla
        "/admin/register"                          # Generic
        "/graphql"                                 # GraphQL introspection
        "/api/v1/user/register"                    # REST API
        "/api/auth/register"                       # REST API
        "/api/signup"                              # REST API
        "/_members/join"                           # Django
        "/accounts/signup"                         # Django allauth
        "/accounts/register"                       # Django
        "/cgi-bin/register.cgi"                    # CGI
    )
    for CMS_PATH in "${CMS_PATHS[@]}"; do
        TEST_URL="https://${HOST}${CMS_PATH}"
        if REG_PAGE=$(check_url "$TEST_URL"); then
            REG_URL="$TEST_URL"
            echo "[+] Registration page found (CMS-specific): $REG_URL"
            break
        fi
    done
fi

# --- If still no registration page, report and exit ---
if [ -z "$REG_PAGE" ]; then
    echo ""
    echo "=== Registration Page Discovery Summary ==="
    echo "[-] No registration page found after exhaustive search"
    echo "[INFO] Methods attempted:"
    echo "  - 30+ common registration paths"
    echo "  - robots.txt parsing"
    echo "  - sitemap.xml parsing"
    echo "  - Homepage link scanning"
    echo "  - Login page registration link detection"
    echo "  - 12 CMS-specific paths"
    echo ""
    echo "[INFO] Possible reasons:"
    echo "  - Site uses external auth (OAuth/SSO only)"
    echo "  - Registration is disabled"
    echo "  - Registration requires specific conditions"
    echo "  - Site is API-only (no registration UI)"
    echo ""
    echo "[VERDICT] No password policy to test - PASS (informational)"
    exit 0
fi

# --- Test 2: Check password field requirements ---
echo ""
echo "=== Test 2: Password Field Analysis ==="
PASSWORD_FIELDS=$(echo "$REG_PAGE" | grep -oiE '<input[^>]*type=["'"'"']password["'"'"'][^>]*>' | head -5)
if [ -n "$PASSWORD_FIELDS" ]; then
    echo "[+] Password fields found"

    # Check for minlength attribute
    MINLENGTH=$(echo "$PASSWORD_FIELDS" | grep -oiE 'minlength="[0-9]+"' | head -1)
    if [ -n "$MINLENGTH" ]; then
        echo "[INFO] $MINLENGTH"
        MIN_VAL=$(echo "$MINLENGTH" | grep -oE '[0-9]+')
        if [ "$MIN_VAL" -lt 8 ]; then
            echo "[FAIL] Minimum password length < 8: $MIN_VAL"
            WEAK_POLICY=1
        elif [ "$MIN_VAL" -ge 12 ]; then
            echo "[PASS] Strong minimum length >= 12"
        else
            echo "[PASS] Minimum length >= 8"
        fi
    else
        echo "[WARN] No minlength attribute found"
        WEAK_POLICY=1
    fi

    # Check for pattern attribute (regex validation)
    PATTERN=$(echo "$PASSWORD_FIELDS" | grep -oiE 'pattern="[^"]*"' | head -1)
    if [ -n "$PATTERN" ]; then
        echo "[PASS] Password pattern validation: $PATTERN"
    else
        echo "[WARN] No password pattern validation"
    fi

    # Check for autocomplete="new-password" (good practice)
    AUTOCOMPLETE=$(echo "$PASSWORD_FIELDS" | grep -oiE 'autocomplete=["'"'"']new-password["'"'"']' | head -1)
    if [ -n "$AUTOCOMPLETE" ]; then
        echo "[PASS] autocomplete=new-password set (good practice)"
    fi
else
    echo "[-] No password fields found on registration page"
fi

# --- Test 3: Check for password requirements text ---
echo ""
echo "=== Test 3: Password Requirements ==="
REQ_KEYWORDS=("must contain" "at least" "uppercase" "lowercase" "number" "special character" "minimum" "complexity" "strength" "password.*required" "require.*password")
REQ_FOUND=0

for KEYWORD in "${REQ_KEYWORDS[@]}"; do
    if echo "$REG_PAGE" | grep -qiE "$KEYWORD"; then
        echo "[+] Password requirement detected: $KEYWORD"
        REQ_FOUND=1
    fi
done

if [ "$REQ_FOUND" -eq 0 ]; then
    echo "[WARN] No password requirements text found on registration page"
    WEAK_POLICY=1
else
    echo "[PASS] Password requirements present"
fi

# --- Test 4: Check for password strength meter ---
echo ""
echo "=== Test 4: Password Strength Meter ==="
STRENGTH_KEYWORDS=("password-strength" "strength-meter" "password-strength-meter" "zxcvbn" "pwscore" "weak.*strong" "password-meter" "meter" "score" "feedback")
STRENGTH_FOUND=0

for KEYWORD in "${STRENGTH_KEYWORDS[@]}"; do
    if echo "$REG_PAGE" | grep -qiE "$KEYWORD"; then
        echo "[+] Password strength indicator detected: $KEYWORD"
        STRENGTH_FOUND=1
        break
    fi
done

if [ "$STRENGTH_FOUND" -eq 0 ]; then
    echo "[WARN] No password strength meter detected"
fi

# --- Test 5: Check for client-side validation only ---
echo ""
echo "=== Test 5: Validation Method ==="
HAS_SERVER_VALIDATION=0
if echo "$REG_PAGE" | grep -qiE 'required.*pattern|pattern.*required|server.*valid|api.*register|/register.*post|submit.*valid|validate.*server'; then
    HAS_SERVER_VALIDATION=1
    echo "[PASS] Server-side validation indicators present"
else
    echo "[WARN] May rely on client-side validation only"
fi

# --- Test 6: Check for CAPTCHA/rate limiting ---
echo ""
echo "=== Test 6: Rate Limiting / CAPTCHA ==="
CAPTCHA_KEYWORDS=("captcha" "recaptcha" "hcaptcha" "turnstile" "rate.limit" "throttle" "lockout" "too.many" "brute.force" "attempt.limit")
CAPTCHA_FOUND=0

for KEYWORD in "${CAPTCHA_KEYWORDS[@]}"; do
    if echo "$REG_PAGE" | grep -qiE "$KEYWORD"; then
        echo "[+] Anti-automation detected: $KEYWORD"
        CAPTCHA_FOUND=1
        break
    fi
done

if [ "$CAPTCHA_FOUND" -eq 0 ]; then
    echo "[WARN] No CAPTCHA or rate limiting detected"
fi

# --- Test 7: Form security analysis ---
echo ""
echo "=== Test 7: Form Security ==="
FORM_TAG=$(echo "$REG_PAGE" | grep -oiE '<form[^>]*>' | head -1)
if [ -n "$FORM_TAG" ]; then
    echo "[INFO] Form found: $(echo "$FORM_TAG" | head -c 120)..."

    # Check method
    if echo "$FORM_TAG" | grep -qi "method=.get"; then
        echo "[WARN] Form uses GET method - credentials may be logged"
        WEAK_POLICY=1
    else
        echo "[PASS] Form uses POST method"
    fi

    # Check for CSRF token
    CSRF=$(echo "$REG_PAGE" | grep -oiE 'csrf|_token|csrfmiddlewaretoken|authenticity_token|__RequestVerificationToken' | head -1)
    if [ -n "$CSRF" ]; then
        echo "[PASS] CSRF protection detected: $CSRF"
    else
        echo "[WARN] No CSRF token detected"
    fi

    # Check for HTTPS form action
    ACTION=$(echo "$FORM_TAG" | grep -oiE 'action=["'"'"'][^"'"'"']*["'"'"']' | head -1)
    if [ -n "$ACTION" ]; then
        if echo "$ACTION" | grep -qi "http:"; then
            echo "[FAIL] Form action uses HTTP (not HTTPS)"
            WEAK_POLICY=1
        else
            echo "[PASS] Form action uses HTTPS"
        fi
    fi
else
    echo "[INFO] No form tag found (may use JavaScript)"
fi

# --- Test 8: Check for password reuse / history policy indicators ---
echo ""
echo "=== Test 8: Password History Policy ==="
HISTORY_KEYWORDS=("password.*history" "recent.*password" "previous.*password" "reuse" "new.*password.*different" "change.*password.*history")
HISTORY_FOUND=0

for KEYWORD in "${HISTORY_KEYWORDS[@]}"; do
    if echo "$REG_PAGE" | grep -qiE "$KEYWORD"; then
        echo "[+] Password history policy detected: $KEYWORD"
        HISTORY_FOUND=1
        break
    fi
done

if [ "$HISTORY_FOUND" -eq 0 ]; then
    echo "[INFO] No password history indicators (registration page typically doesn't show this)"
fi

# --- Test 9: Check for MFA/2FA indicators ---
echo ""
echo "=== Test 9: MFA/2FA Indicators ==="
MFA_KEYWORDS=("two.factor" "2fa" "multi.factor" "mfa" "authenticator" "totp" "sms.*code" "email.*code" "verification.*code")
MFA_FOUND=0

for KEYWORD in "${MFA_KEYWORDS[@]}"; do
    if echo "$REG_PAGE" | grep -qiE "$KEYWORD"; then
        echo "[+] MFA/2FA indicator detected: $KEYWORD"
        MFA_FOUND=1
        break
    fi
done

if [ "$MFA_FOUND" -eq 0 ]; then
    echo "[INFO] No MFA/2FA indicators on registration page"
fi

# --- Summary ---
echo ""
echo "=== Summary ==="
echo "Registration page: $REG_URL"
if [ "$WEAK_POLICY" -eq 1 ]; then
    echo "[FAIL] Weak password policy indicators detected"
    exit 1
else
    echo "[PASS] Password policy appears adequate"
    exit 0
fi
