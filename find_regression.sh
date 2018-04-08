#! /bin/sh

set -e

if [ $# -lt 1 ]; then
	echo Need test
	exit 1
fi

timeout() {
	local cmd= sleep= rv=

	"$@" 2>&1 &
	cmd=$!
	sleep 1 &
	sleep=$!
	wait -n 2>/dev/null
	rv=$?
	kill $cmd 2> /dev/null && echo " finished in 1000.0"
	kill $sleep 2> /dev/null
	wait 2>/dev/null
	return $rv
}

BENCH_JSON=$PWD/bench/bench-file.json
BENCH_PLIST=$PWD/bench/bench-file.plist
plotfile=$(mktemp --tmpdir find_regression_plot.XXXXXXXXXX)
make "$BENCH_JSON" "$BENCH_PLIST"
head=$(git rev-parse HEAD)
CFLAGS="$(grep '^CFLAGS[^_A-Z]' config.mk | cut -d= -f 2-) -Wno-error"

rm -rf find_regression
git clone . find_regression
cd find_regression

git checkout $head 2> /dev/null


git log --reverse --format='%h' "HEAD~10..HEAD" | while read -r hash; do
	git checkout .
	git checkout "$hash" 2> /dev/null
	git clean -f 2> /dev/null
	sed -i 's/-D\(BENCH_[A-Z]*\)="[^"]*"/-D\1="$(\1)"/' Makefile
	if ! make clean "$1" \
		BENCH_JSON=$BENCH_JSON \
		BENCH_PLIST="$BENCH_PLIST" \
		CFLAGS="$CFLAGS" > /dev/null 2>&1; then
		printf '%s\t0\t%s\n' "$hash" "Build Error"
		continue
	fi

	# make sure we run against the right bench file

	find . -name '*.json'  -print0 | xargs -r0 rm
	find . -name '*.plist' -print0 | xargs -r0 rm

	#find . -name '*.json'  -print0 | xargs -r0 -L 1 cp "$BENCH_JSON"
	#find . -name '*.plist' -print0 | xargs -r0 -L 1 cp "$BENCH_PLIST"

	#run for warmup
	for i in $(seq 1 10); do
		result=$("$@" 2>&1 | grep '^ finished in' | sed 's/^[^0-9.]*//; s/[^0-9.].*//;')
		#result=$(timeout "$@" | grep '^ finished in' | sed 's/^[^0-9.]*//; s/[^0-9.].*//;')
	done

	if ! [ "$result" ]; then
		result=0
		msg="Run Error"
	fi
	printf '%s\t%s\t%s\n' "$hash" "$result" "$msg"
done | tee "$plotfile"

cd .. || exit 1
rm -rf find_regression

gnuplot - <<EOF
set term png
set output '$plotfile.png'
plot '$plotfile' using 0:2 with lines title ""
EOF
xdg-open $plotfile.png
