.PHONY: all clean colorShader textureShader

all: colorShader textureShader

colorShader:
	./latte-assembler assemble --vsh=colorShader.vsh --psh=colorShader.psh colorShader.gsh

textureShader:
	./latte-assembler assemble --vsh=textureShader.vsh --psh=textureShader.psh textureShader.gsh

clean:
	rm -f *.gsh
