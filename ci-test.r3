Rebol [
    title: "Rebol/Triangulate extension CI test"
]

print ["Running test on Rebol build:" mold to-block system/build]

system/options/quiet: false
system/options/log/rebol: 4

CI?: any [
    "true" = get-env "CI"
    "true" = get-env "GITHUB_ACTIONS"
    "true" = get-env "TRAVIS"
    "true" = get-env "CIRCLECI"
    "true" = get-env "GITLAB_CI"
]

if CI? [
    ;; for the CI test the module is the build directory 
    system/options/modules: what-dir
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

? out

print-horizontal-line
do %test/triangulate-image.r3

print-horizontal-line
do %test/triangulate-spiral.r3

print-horizontal-line
print-horizontal-line
print as-green "DONE"

