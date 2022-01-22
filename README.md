# Median

Linux/AviSynth+ port of the Median filter originally developed by Antti Korhola
and modded by TomArrow

Avisynth wiki: http://avisynth.nl/index.php/Median

Forum: https://forum.doom9.org/showthread.php?t=170216

Build instructions
==================
Linux
  
* Clone repo and build
    
        git clone https://github.com/m0qui/Median.git
        cd Median
        cmake -B build -S .
        cmake --build build

  Useful hints:        
   build after clean:

        cmake --build build --clean-first

   delete cmake cache

        rm build/CMakeCache.txt

* Find binaries at
    
        build/libmedian.so

* Install binaries

        cd build
        sudo make install
  
