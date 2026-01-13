EXEC0 = CppAudioCapture

EXEC1 = CppAudioCaptureSpectre

EXEC2 = CppAudioCaptureSelect


CLIB0 = -I./lib/portaudio/include ./lib/portaudio/lib/.libs/libportaudio.a -lrt -lasound -ljack -pthread
CLIB1 = -I./lib/portaudio/include ./lib/portaudio/lib/.libs/libportaudio.a -lrt -lasound -ljack -pthread -I./lib/fftw-3.3.10/api -lfftw3

$(EXEC0): CppAudioCapture.cpp
	g++ -o $@ $^ $(CLIB0)
.PHONY: $(EXEC0)

$(EXEC1): CppAudioCaptureSpectre.cpp
	g++ -o $@ $^ $(CLIB1)
.PHONY: $(EXEC1)

$(EXEC2): CppAudioCaptureSelect.cpp
	g++ -o $@ $^ $(CLIB0)
.PHONY: $(EXEC2)

install-deps: install-portaudio install-fftw
.PHONY: install-deps

uninstall-deps: uninstall-portaudio uninstall-fftw
.PHONY: uninstall-deps

install-portaudio:
	mkdir -p lib
	curl https://files.portaudio.com/archives/pa_stable_v190700_20210406.tgz | tar -zx -C lib
	cd lib/portaudio && ./configure && $(MAKE) -j
.PHONY: install-portaudio

uninstall-portaudio:
	cd lib/portaudio && $(MAKE) uninstall
	rm -rf lib/portaudio
.PHONY: uninstall-portaudio

install-fftw:
	mkdir -p lib
	curl http://www.fftw.org/fftw-3.3.10.tar.gz | tar -zx -C lib
	cd lib/fftw-3.3.10 && ./configure && $(MAKE) -j
.PHONY: install-fftw

uninstall-fftw:
	cd lib/fftw-3.3.10 && $(MAKE) uninstall
	rm -rf lib/fftw-3.3.10
.PHONY: uninstall-fftw

all: $(EXEC0) $(EXEC1) $(EXEC2)
.PHONY: all

clean:
	rm -f $(EXEC0)
	rm -f $(EXEC1)
	rm -f $(EXEC2)
.PHONY: clean

clean-all: clean
	rm -rf lib
.PHONY: clean-all
