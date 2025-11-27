# iqrfgd2(1) completion                                    -*- shell-script -*-

_comp_cmp_iqrfgd2()
{
  local cur prev opts
  COMPREPLY=()
  cur="${COMP_WORDS[COMP_CWORD]}"
  prev="${COMP_WORDS[COMP_CWORD-1]}"

  # all available options
  opts="
        -h --help
        -v --version
        -c --configuration
        -p --pidfile
    "

  # helper: generate file completions by extension
  _complete_files_by_ext() {
    local ext="$1"
    compopt -o filenames
    if [[ "$cur" == /* || "$cur" == ./* || "$cur" == ../* ]]; then
      mapfile -t COMPREPLY < <(compgen -G "${cur}*${ext}")
    else
      mapfile -t COMPREPLY < <(compgen -G "*${ext}")
    fi
  }

  case "$prev" in
  -c|--configuration)
    _complete_files_by_ext ".json"
    return 0
    ;;
  -p|--pidfile)
    _complete_files_by_ext ".pid"
    return 0
    ;;
  esac

  # complete option names
  if [[ "$cur" == -* ]]; then
    mapfile -t COMPREPLY < <(compgen -W "${opts}" -- "$cur")
    return 0
  fi
}

complete -F _comp_cmp_iqrfgd2 iqrfgd2

# ex: filetype=sh
