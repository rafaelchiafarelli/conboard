#!/usr/bin/env bash
#
# Show or reset the conboard console's web login (nginx Basic Auth in front of
# the whole site, see backend/assets/interface.conf). Installed to
# /usr/local/bin/conboard-password by install-on-device.sh, so it's always on
# PATH regardless of where the artifact was unpacked.
#
#   conboard-password            # print the current password (needs root to read it)
#   sudo conboard-password --reset   # generate + install a brand new one
#
# Exists so losing the password is never a real lockout: install-on-device.sh
# calls this same script (with --reset) to generate the very first password, so
# there's exactly one code path to keep correct, and the same recovery command
# works whether the plaintext file was lost, the login was forgotten, or you
# just want to rotate it.
set -euo pipefail

HTPASSWD_FILE=/etc/nginx/.htpasswd-conboard
PASSWORD_FILE=/etc/conboard-web-password.txt

reset_password() {
    if [ "$EUID" -ne 0 ]; then
        echo "Resetting the password requires root: sudo conboard-password --reset" >&2
        exit 1
    fi
    command -v openssl >/dev/null 2>&1 || {
        echo "openssl not found -- can't generate a password. Install openssl and retry." >&2
        exit 1
    }
    # nginx workers run as an unprivileged user (usually www-data, not guaranteed) --
    # the htpasswd file has to be readable by THAT group or every authenticated
    # request 500s (found the hard way against real hardware, see NOTES.md).
    local nginx_user nginx_group web_password
    nginx_user="$(awk '/^[ \t]*user[ \t]+/{print $2; exit}' /etc/nginx/nginx.conf 2>/dev/null | tr -d ';')"
    nginx_user="${nginx_user:-www-data}"
    nginx_group="$(id -gn "$nginx_user" 2>/dev/null || echo "$nginx_user")"

    web_password="$(openssl rand -base64 18 | tr -d '=+/' | head -c 24)"
    printf 'conboard:%s\n' "$(openssl passwd -apr1 "$web_password")" > "$HTPASSWD_FILE"
    chown "root:$nginx_group" "$HTPASSWD_FILE" 2>/dev/null || true
    chmod 640 "$HTPASSWD_FILE"
    printf 'user: conboard\npassword: %s\n' "$web_password" > "$PASSWORD_FILE"
    chmod 600 "$PASSWORD_FILE"

    # A durable trail in the journal in case the terminal output that showed the
    # password scrolls away or the install was non-interactive -- deliberately
    # does NOT include the password itself (journalctl access can be broader than
    # root on some distros); the actual secret only ever lives in the two files
    # above and whatever this command just printed to your terminal.
    logger -t conboard "web login password (re)generated -- retrieve with: sudo conboard-password"

    echo "New password generated."
    echo ">>> web login -- user: conboard  password: $web_password <<<"
    echo "(also saved to $PASSWORD_FILE, root-only -- retrieve any time with: sudo conboard-password)"
}

show_password() {
    if [ ! -f "$PASSWORD_FILE" ]; then
        echo "No password file at $PASSWORD_FILE (maybe never generated, or lost)." >&2
        echo "Fix it: sudo conboard-password --reset" >&2
        exit 1
    fi
    if [ ! -r "$PASSWORD_FILE" ]; then
        echo "Permission denied reading $PASSWORD_FILE -- retry with sudo: sudo conboard-password" >&2
        exit 1
    fi
    cat "$PASSWORD_FILE"
}

case "${1:-}" in
    --reset) reset_password ;;
    "")      show_password ;;
    *)       echo "usage: conboard-password [--reset]" >&2; exit 2 ;;
esac
