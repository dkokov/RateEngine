#!/bin/sh

DIR=`pwd`

CONV=`which pandoc`

# contert from md files to pdf file
genDoc_pdf()
{
#    $CONV README.md LICENSE.md doc/*.md --toc --number-sections --top-level-division=part -o rt_v7.pdf
    $CONV README.md LICENSE.md doc/*.md -o rt_v7.pdf
}

# contert from md files to html file
genDoc_html()
{
    $CONV README.md LICENSE.md doc/*.md -o rt_v7.html
}

if [ -f "$CONV" ];
then
    if [ $# -eq 0 ];
    then
	genDoc_pdf
    else
	if [ $1 == "html" ];
	then
#	    tmp=$DIR"/doc/html/"
#	    mkdir $tmp
#	    cp -vfR $DIR"/doc/png/" $tmp"/png/"
	    genDoc_html
	fi
    fi
else
    echo "Sould be installed 'pandoc'"
    exit
fi

#pandoc README.md  --toc --number-sections --top-level-division=part --output=rt_v7.pdf

