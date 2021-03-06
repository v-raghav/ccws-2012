// Copyright (c) 2009-2011, Tor M. Aamodt, Wilson W.L. Fung
// The University of British Columbia
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
// Neither the name of The University of British Columbia nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once 

// detection and fallback for unordered_map in C++0x
#ifdef __cplusplus
   #ifdef __GNUC__
       #if __GNUC__ >= 4 && __GNUC_MINOR__ >= 3
          #include <unordered_map>
          #define tr1_hash_map std::unordered_map
          #define tr1_hash_map_ismap 0
       #else
          #include <map>
          #define tr1_hash_map std::map
          #define tr1_hash_map_ismap 1
       #endif
   #else
      #include <map>
      #define tr1_hash_map std::map
      #define tr1_hash_map_ismap 1
   #endif

    // detect gcc 4.3 and use unordered map (part of c++0x)
    // unordered map doesn't play nice with _GLIBCXX_DEBUG, just use a map if its enabled.
    #if  defined( __GNUC__ ) and not defined( _GLIBCXX_DEBUG )
        #if __GNUC__ >= 4 && __GNUC_MINOR__ >= 3
           #include <unordered_map>
           #define my_hash_map std::unordered_map
        #else
           #include <ext/hash_map>
           namespace std {
              using namespace __gnu_cxx;
           }
           #define my_hash_map std::hash_map
        #endif
    #else
       #include <map>
       #define my_hash_map std::map
       #define USE_MAP
    #endif

#endif
