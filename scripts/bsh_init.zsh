# scripts/bsh_init.zsh

# --- Configuration ---
# TODO: Double check this path matches your actual build location
BSH_BINARY="$HOME/bsh/build/bsh"


zmodload zsh/datetime

# --- Internal Variables ---
typeset -g _bsh_start_time
typeset -g _bsh_current_cmd

# 1. Executed BEFORE the command runs
_bsh_preexec() {
    # Capture high-precision start time
    _bsh_start_time=$EPOCHREALTIME
    _bsh_current_cmd="$1"
}

# 2. Executed AFTER the command finishes
_bsh_precmd() {
    # Capture exit code immediately
    local exit_code=$?
    
    # Calculate duration
    local duration=0
    if [[ -n "$_bsh_start_time" ]]; then
        local now=$EPOCHREALTIME
        # Calculate diff in ms, zsh handles the float math
        local duration_float=$(( (now - _bsh_start_time) * 1000 ))
        # Truncate to integer
        duration=${duration_float%.*} 
    fi

    # Record to BSH
    if [[ -n "$_bsh_current_cmd" && -x "$BSH_BINARY" ]]; then
        "$BSH_BINARY" record \
            --cmd "$_bsh_current_cmd" \
            --cwd "$PWD" \
            --exit "$exit_code" \
            --duration "$duration" \
            --session "$$" &!
    fi

    # Reset variables
    unset _bsh_current_cmd
    unset _bsh_start_time
}

# 3. Hook into Zsh
autoload -Uz add-zsh-hook
add-zsh-hook preexec _bsh_preexec
add-zsh-hook precmd _bsh_precmd