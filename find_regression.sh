#! /bin/sh

BENCH_JSON=bench/bench-file.json
BENCH_PLIST=bench/bench-file.plist
RANGE="HEAD~20..HEAD"
#RANGE="099a41b..HEAD"
CFLAGS="$(grep '^CFLAGS[^_A-Z]' config.mk | cut -d= -f 2-) -Wno-error -std=gnu11"
WARMUP=0

###############################

set -e
if [ $# -lt 1 ]; then
	echo Need test
	exit 1
fi

cd "$(dirname $(which "$0"))"

plotfile=$(mktemp --tmpdir find_regression_plot.XXXXXXXXXX)
make "$BENCH_JSON" "$BENCH_PLIST"
head=$(git rev-parse HEAD)

BENCH_JSON=$(realpath "$BENCH_JSON")
BENCH_PLIST=$(realpath "$BENCH_PLIST")
rm -rf find_regression
git clone . find_regression
cd find_regression

git checkout $head 2> /dev/null

git log --reverse --format='%h' "$RANGE" | while read -r hash; do
	msg=
	git checkout .
	git checkout "$hash" 2> /dev/null
	git clean -f 2> /dev/null
	sed -i 's/-D\(BENCH_[A-Z]*\)="[^"]*"/-D\1="$(\1)"/' Makefile
	make clean > /dev/null > /dev/null 2>&1
	ln -fs $BENCH_JSON $BENCH_JSON.run
	ln -fs $BENCH_PLIST $BENCH_PLIST.run
	output=$(make "$1" \
		BENCH_JSON="$BENCH_JSON.run" \
		BENCH_PLIST="$BENCH_PLIST.run" \
		CFLAGS="$CFLAGS" > /dev/null 2>&1)
	if [ "$?" -ne 0 ]; then
		printf '%s\t0\t%s\n' "$hash" "Build Error"
		echo "$output" >&2
		continue
	fi

	# make sure we run against the right bench file

	find . -name '*.json'  -print0 | xargs -r0 rm
	find . -name '*.plist' -print0 | xargs -r0 rm

	#find . -name '*.json'  -print0 | xargs -r0 -L 1 cp "$BENCH_JSON"
	#find . -name '*.plist' -print0 | xargs -r0 -L 1 cp "$BENCH_PLIST"

	#run for warmup
	for i in $(seq 1 $WARMUP) run; do
		result=$("$@" 2>&1 | grep '^ finished in' | sed 's/^[^0-9.]*//; s/[^0-9.].*//;' | head -n 1)
		#result=$(timeout "$@" | grep '^ finished in' | sed 's/^[^0-9.]*//; s/[^0-9.].*//;')
	done

	if ! [ "$result" ]; then
		result=0
		msg="Run Error"
	fi
	printf '%s\t%s\t%s\n' "$hash" "$result" "$msg"
done | tee "$plotfile"

cd .. || exit 1
rm -rf find_regression $BENCH_JSON.run $BENCH_PLIST.run

gnuplot - <<EOF
set term png
set output '$plotfile.png'
set xtics rotate by -45
plot '$plotfile' using 0:2:xtic(1) with lines title ""
EOF
xdg-open $plotfile.png
