Rebol [
    title: "Rebol/Triangulate extension CI test"
]

print ["Running test on Rebol build:" mold to-block system/build]

system/options/quiet: false
system/options/log/rebol: 4

if CI?: any [
    "true" = get-env "CI"
    "true" = get-env "GITHUB_ACTIONS"
    "true" = get-env "TRAVIS"
    "true" = get-env "CIRCLECI"
    "true" = get-env "GITLAB_CI"
][
    ;; configure modules location for the CI test 
    system/options/modules: dirize to-rebol-file any [
    	get-env 'REBOL_MODULES_DIR
    	what-dir
    ]
    ;; make sure that we load a fresh extension
	try [system/modules/triangulate: none]
]

import triangulate

;- the `inp` object must have at least `points` as a vector with decimal values
inp: object [
   points: #(f64! [
       0.0   0.0
       10.0  0.0
       10.0  10.0
       0.0   10.0
       2.5   2.5
   ])
   report: true ;; will print triangulation info
]

out: object [
	points:          none
	attributes:      none
	markers:         none
	segments:        none
	segment-markers: none
	edges:           none
	triangles:       none

	; voronoi output:
	v-points:        none
	v-attributes:    none
	v-edges:         none
	v-norms:         none  
]

triangulate inp out ;; the triangulation request

;- the `out` object should now contain the result of the triangulation

assert [ out/points          == #(f64! [0.0 0.0 10.0 0.0 10.0 10.0 0.0 10.0 2.5 2.5])]
assert [ out/attributes      == none]
assert [ out/markers         == #(i32! [1 1 1 1 0])]
assert [ out/segments        == #(i32! [1 0 2 1 3 2 0 3])]
assert [ out/segment-markers == #(i32! [1 1 1 1])]
assert [ out/edges           == #(i32! [3 0 0 4 4 3 4 1 1 2 2 4 0 1 2 3])]
assert [ out/triangles       == #(i32! [3 0 0 4 4 3 4 1 1 2 2 4])]
assert [ out/v-points        == #(f64! [-2.5 5.0 7.5 5.0 5.0 -2.5 5.0 7.5])]
assert [ out/v-attributes    == none]
assert [ out/v-edges         == #(i32! [0 -1 0 2 0 3 1 2 1 -1 1 3 2 -1 3 -1])]
assert [ out/v-norms         == #(f64! [-10.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0])]

;; The following scripts require the Blend2D extension, which may not be available on all platforms.
;; Only execute these scripts if the import does not fail.

try/with [import 'blend2d][
	print "Failed to import Rebol/Blend2D extension!"
	print "Skipping some tests!"
	quit
]

print-horizontal-line
do %test/triangulate-image.r3

print-horizontal-line
do %test/triangulate-spiral.r3

print-horizontal-line
print-horizontal-line
print as-green "DONE"

