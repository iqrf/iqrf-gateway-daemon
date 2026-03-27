# iqrfgd2-tokenctl(1) completion                                      -*- shell-script -*-

_iqrfgd2-tokenctl() {
  local curr prev
  curr="${COMP_WORDS[COMP_CWORD]}"

  # available commands
  local commands="
    help
    list
    get
    create
    revoke
    rotate
  "
  # command options
  local shared_opts="-h --help -p --path"
  local list_opts="$shared_opts"
  local get_rotate_opts="$shared_opts -i --id -j --json"
  local create_opts="$shared_opts -o --owner -e --expiration -j --json"
  local revoke_opts="$shared_opts -i --id"

  # no command specified yet
  if [[ $COMP_CWORD == 1 ]]; then
    COMPREPLY=( $(compgen -W "$commands" -- "$curr") )
    return 0
  fi

  # command received
  cmd="${COMP_WORDS[1]}"
  curr="${COMP_WORDS[COMP_CWORD]}"
  prev="${COMP_WORDS[COMP_CWORD-1]}"

  # help found, stop completing
  for token in "${COMP_WORDS[@]}"; do
    case "$token" in
      -h|--help)
        COMPREPLY=()
        return
        ;;
    esac
  done

  # if specifying database file, look for db files
  case "$prev" in
    -p|--path)
      local files=( $(compgen -f -- "$curr") )
      COMPREPLY=()

      for file in "${files[@]}"; do
        if [[ -d "$file" ]]; then
          compopt -o nospace 2>/dev/null
          COMPREPLY+=( "$file/" )
          continue
        fi

        if [[ "$file" == *.db ]]; then
          COMPREPLY+=( "$file" )
        fi
      done
      return 0
      ;;
  esac

  case "$cmd" in
    help)
      COMPREPLY=()
      return
      ;;
    list)
      COMPREPLY=( $(compgen -W "$list_opts" -- "$curr") )
      return
      ;;
    get|rotate)
      COMPREPLY=( $(compgen -W "$get_rotate_opts" -- "$curr") )
      return
      ;;
    create)
      COMPREPLY=( $(compgen -W "$create_opts" -- "$curr") )
      return
      ;;
    revoke)
      COMPREPLY=( $(compgen -W "$revoke_opts" -- "$curr") )
      return
      ;;
  esac
}

complete -F _iqrfgd2-tokenctl iqrfgd2-tokenctl

# ex: filetype=sh
