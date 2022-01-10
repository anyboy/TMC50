#!/bin/bash -e

##--------------------------------------------------------
##-- generate patch between two manifest-xml or commits
##--   $1 [in] src ( manifest-xml or commit )
##--   $2 [in] dst ( manifest-xml or commit )
##--   $3 [in] src dir
##--   $4 [out] out dir
##-- example:
##--   ./xmldiff.sh TAG1.xml TAG2.xml ../src ./output
##--   ./xmldiff.sh a670b287 adc7d68f ../src ./output
##--------------------------------------------------------
print_usage(){
	echo "Usage:"
    echo "  $0 src-xml dst-xml src-dir out-dir"
    echo "  $0 src-commit dst-commit src-dir out-dir"
	echo
}

##--------------------------------------------------------
##-- generate git-revision function
##--   $1 [in] src manifest-xml
##--   $2 [in] dst manifest-xml
##--   $3 [out] rev file [path rev1 rev2]
##--------------------------------------------------------
git_rev(){
	# Fix path
	FIX_PATHREV='s/^\(.*\)name="\([^\ ]*\)"[^=]*revision=\(.*\)$/\1name="\2" path="\2" revision=\3/g'
	sed -i "$FIX_PATHREV" $1 && sed -i "$FIX_PATHREV" $2

	# Get [path revision]
	GET_PATHREV='s/^.*name="\([^\ ]*\)".*path="\([^\ ]*\)".*revision="\([^\ ]*\)".*$/\2 \3/p'
	sed -n "$GET_PATHREV" $1 > tmp1 && sed -n "$GET_PATHREV" $2 > tmp2

	# Create dir
	mkdir -p $(dirname $3)
	echo -n "" > $3

	# Combine [path rev1 rev2]
	echo -e "$3\n"
	while read -r path rev1; do
		rev2=$(awk "\$1==\"$path\" {print\$2}" tmp2)
		# we should compare both revisions and decide weather to output
		if [ x${rev1} != x ] && [ x${rev2} != x ] && [ "$rev1" != "$rev2" ]; then
			echo "$path $rev1 $rev2" | tee -a $3
			#echo -e "$path $rev1 $rev2"
		fi
	done < tmp1

	rm -rf tmp1 tmp2
}

##--------------------------------------------------------
##-- generate git-diff function
##--   $1 [in] rev file [path rev1 rev2]
##--   $2 [in] src dir
##--   $3 [out] output dir
##--------------------------------------------------------
git_diff(){
	# Create dir
	mkdir -p $3
	rm -rf $3/*

	# Diff output
	while read -r path rev1 rev2; do
		spath=${path//\//_}
		cd $2/$path
		git diff $rev1..$rev2 > $3/$spath.diff
		echo -e "$3/$spath.diff"
	done < $1
}

##--------------------------------------------------------
##-- generate git-log function
##--   $1 [in] rev file [path rev1 rev2]
##--   $2 [in] src dir
##--   $3 [out] output file
##--------------------------------------------------------
git_log(){
	# Cmd param
	TITLE="Path,Author,Date,Subject,Commit"
	FORMAT="%an,%ai,\"%s\",%H%n"

	# Create dir
	mkdir -p $(dirname $3)
	echo $TITLE > $3

	# Log output
	echo -e "$3"
	while read -r path rev1 rev2; do
		cd $2/$path
		git log $rev1..$rev2 --pretty=format:"$path,$FORMAT" >> $3
	done < $1

	# Format csv
	sed -i -e '/^$/d' -e 's/"/""/g' -e 's/,""/,"/g' -e 's/"",/",/g' $3
}

##--------------------------------------------------------
##-- generate git-patch function
##--   $1 [in] rev file [path rev1 rev2]
##--   $2 [in] src dir
##--   $3 [out] output dir
##--------------------------------------------------------
git_patch(){
	# Create dir
	mkdir -p $3
	rm -rf $3/*

	# Patch output
	while read -r path rev1 rev2; do
		spath=${path//\//_}
		cd $2/$path
		mkdir -p $3/$spath
		git format-patch -o $3/$spath $rev1..$rev2
	done < $1
}

##--------------------------------------------------------
##-- generate git-file function
##--   $1 [in] rev file [path rev1 rev2]
##--   $2 [in] src dir
##--   $3 [out] output dir
##--------------------------------------------------------
git_file(){
	# Create dir
	mkdir -p $3
	rm -rf $3/*
	echo -n "" > $3.txt
	echo -e "$3.txt"

	# file output
	while read -r path rev1 rev2; do
		mkdir -p $3/old/$path
		mkdir -p $3/new/$path
		cd $2/$path
		echo -e "\n$2/$path" | tee -a $3.txt

		git diff --name-status --oneline $rev1..$rev2 | tee $3/tmp1 | tee -a $3.txt

		while read -r mode file; do
			if [ "$mode" = "M" -o "$mode" = "D" ]; then
				mkdir -p $(dirname $3/old/$path/$file)
				if [ -f "$file" ]; then
					git show $rev1:$file > $3/old/$path/$file
				elif [ -d "$file" ]; then
					echo "bypass $file"
				fi
			fi
			if [ "$mode" = "M" -o "$mode" = "A" ]; then
				mkdir -p $(dirname $3/new/$path/$file)
				if [ -f "$file" ]; then
					git show $rev2:$file > $3/new/$path/$file
				elif [ -d "$file" ]; then
					echo "bypass $file"
				fi
			fi
		done < $3/tmp1
		
		rm -rf $3/tmp1
	done < $1
}

##--------------------------------------------------------
##-- main function
##--------------------------------------------------------
SRCDIR=$(cd $3; pwd)
OUTDIR=$(mkdir -p $4; cd $4; pwd)
GITDIFF_TXT=$OUTDIR/gits-diff.txt

# check params
if [ $# -lt 4 ]; then
    print_usage
    exit
fi

echo -e "\n--== Task start ==--\n"
echo -e "SRC: $1\nDST: $2\nSRCDIR: $SRCDIR\nOUTDIR: $OUTDIR"

if [ "${1##*.}" = "xml" ]; then
	# repo manifest-xml
	echo -e "\n\n--== Generating repo diff info ==--\n"
	git_rev $1 $2 $GITDIFF_TXT
else
	# git commits
	echo -e "\n\n--== Generating git diff info ==--\n"
	path=${SRCDIR##*/}
	SRCDIR=${SRCDIR%/*}
	echo "$path $1 $2" | tee $GITDIFF_TXT
fi

echo -e "\n\n--== Generating git diff patch ==--\n"
git_diff $GITDIFF_TXT $SRCDIR $OUTDIR/gits-diff

echo -e "\n\n--== Generating git patch info ==--\n"
git_log $GITDIFF_TXT $SRCDIR $OUTDIR/gits-patch.csv

echo -e "\n\n--== Generating git format patch ==--\n"
git_patch $GITDIFF_TXT $SRCDIR $OUTDIR/gits-patch

echo -e "\n\n--== Generating git file patch ==--\n"
git_file $GITDIFF_TXT $SRCDIR $OUTDIR/gits-file

echo -e "\n--== Task complete ==--\n"
