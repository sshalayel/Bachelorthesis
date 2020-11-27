# starts an pandora environment
# source this file

limit=75
limitkb=$((limit * 1024 ** 2))
#limit ram to 50 Gb
ulimit -v $limitkb
echo "Limited to $limitkb Kb($limit Gb) RAM!"

echo 'When started with tmux : detach with Ctrl-b and d and reattach with tmux attach'

function MAKE { make CC="g++-7 -DPANDORA"; }
