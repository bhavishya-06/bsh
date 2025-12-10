# scripts/bsh_init.zsh

BSH_BINARY="$HOME/bsh/build/bsh"

# State Variables
typeset -gA _bsh_suggestions
typeset -g _bsh_start_time
typeset -g _bsh_mode=0  # 0=Global, 1=Directory, 2=Branch

# --- GIT HELPER ---
_bsh_get_branch() {
    git rev-parse --abbrev-ref HEAD 2>/dev/null
}

# --- SUGGESTION ENGINE ---
_bsh_refresh_suggestions() {
    if [[ ! -x "$BSH_BINARY" ]]; then return; fi
    # Only run if buffer has content
    if [[ -z "$BUFFER" || ${#BUFFER} -lt 1 ]]; then
        POSTDISPLAY=""
        return
    fi

    # 1. Prepare Arguments
    local args=("$BUFFER" "--scope")
    local header_text=" BSH: Global "
    
    if [[ $_bsh_mode -eq 0 ]]; then
        args+=("global")
    elif [[ $_bsh_mode -eq 1 ]]; then
        args+=("dir" "--cwd" "$PWD")
        header_text=" BSH: Directory "
    elif [[ $_bsh_mode -eq 2 ]]; then
        local branch=$(_bsh_get_branch)
        if [[ -z "$branch" ]]; then
            _bsh_mode=0; args+=("global")
        else
            args+=("branch" "--branch" "$branch")
            header_text=" BSH: Branch ($branch) "
        fi
    fi

    # 2. Query BSH
    local output
    output=$("$BSH_BINARY" suggest "${args[@]}")
    
    if [[ -z "$output" ]]; then
        POSTDISPLAY=""
        _bsh_suggestions=()
        return
    fi

    # 3. Parse & Build UI
    _bsh_suggestions=()
    local -a display_lines
    local max_len=${#header_text}
    local i=0
    
    echo "$output" | while read -r line; do
        [[ -z "$line" ]] && continue
        [[ $i -ge 5 ]] && break 

        _bsh_suggestions[$i]="$line"
        
        # Display: " ⌥1: command "
        local text=" ⌥$i: $line"
        display_lines+=("$text")
        
        if (( ${#text} > max_len )); then max_len=${#text}; fi
        ((i++))
    done

    # 4. Render Box
    local result=$'\n'
    local top="╭$header_text"
    for ((k=${#header_text}; k<max_len+1; k++)); do top+="─"; done
    top+="╮"
    result+="$top"

    for line in "${display_lines[@]}"; do
        result+=$'\n'
        local pad=$(( max_len - ${#line} ))
        local padding=""
        for ((k=0; k<pad+1; k++)); do padding+=" "; done
        result+="│$line$padding│"
    done
    
    result+=$'\n'
    local bot="╰"
    for ((k=0; k<max_len+1; k++)); do bot+="─"; done
    bot+="╯"
    result+="$bot"

    POSTDISPLAY="$result"
}

# --- STATE SWITCHER (Alt + Arrows) ---
# Supports both Standard and MacOS keycodes
_bsh_cycle_mode_fwd() { (( _bsh_mode = (_bsh_mode + 1) % 3 )); _bsh_refresh_suggestions; zle -R; }
_bsh_cycle_mode_back() { (( _bsh_mode = (_bsh_mode - 1) )); if (( _bsh_mode < 0 )); then _bsh_mode=2; fi; _bsh_refresh_suggestions; zle -R; }

zle -N _bsh_cycle_mode_fwd
zle -N _bsh_cycle_mode_back

bindkey "^[f" _bsh_cycle_mode_fwd       # Standard Alt+Right
bindkey "^[[1;3C" _bsh_cycle_mode_fwd   # iTerm Alt+Right
bindkey "^[b" _bsh_cycle_mode_back      # Standard Alt+Left
bindkey "^[[1;3D" _bsh_cycle_mode_back  # iTerm Alt+Left

# --- STANDARD HOOKS ---
_bsh_self_insert() { zle .self-insert; _bsh_refresh_suggestions; }
zle -N self-insert _bsh_self_insert

_bsh_backward_delete_char() { zle .backward-delete-char; _bsh_refresh_suggestions; }
zle -N backward-delete-char _bsh_backward_delete_char

# --- EXECUTION BINDINGS (The Fix) ---

_bsh_run_idx() {
    local i=$1
    if [[ -n "${_bsh_suggestions[$i]}" ]]; then
        BUFFER="${_bsh_suggestions[$i]}"
        POSTDISPLAY=""
        zle .accept-line
    fi
}

# Create widgets for 0-4
for i in {0..4}; do
    eval "_bsh_run_$i() { _bsh_run_idx $i; }; zle -N _bsh_run_$i"
done

# 1. Standard Escape Sequences (Linux / Configured Mac)
bindkey '^[0' _bsh_run_0
bindkey '^[1' _bsh_run_1
bindkey '^[2' _bsh_run_2
bindkey '^[3' _bsh_run_3
bindkey '^[4' _bsh_run_4

# 2. MacOS Native Option Key Symbols (The Magic Fix)
# 0=º, 1=¡, 2=™, 3=£, 4=¢ (Standard US Layout)
bindkey 'º' _bsh_run_0
bindkey '¡' _bsh_run_1
bindkey '™' _bsh_run_2
bindkey '£' _bsh_run_3
bindkey '¢' _bsh_run_4