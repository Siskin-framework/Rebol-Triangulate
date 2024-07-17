Rebol [
   title: "Delaunay triangulation of a set of vertices"
   needs: 3.11.0 ;; needed for an automatic extension downloads
]

CI?: any [
   "true" = get-env "CI"
   "true" = get-env "GITHUB_ACTIONS"
   "true" = get-env "TRAVIS"
   "true" = get-env "CIRCLECI"
   "true" = get-env "GITLAB_CI"
]

if CI? [
   ;; make sure that we load a fresh extension
   try [system/modules/triangulate: none]
   ;; use project's root directory as a modules location
   system/options/modules: dirize to-real-file %../
]

import 'triangulate
import 'blend2d

inp: object [
   ; spiral shaped input points
   points: #(f64! [
      390.0 390.0
      327.6 526.4
      187.5 455.4
      144.0 307.7
      193.5 163.5
      310.2  64.5
      458.1  28.5
      607.5  58.5
      733.5 141.0
      822.0 264.3
      864.0 409.7
      858.0 561.0
      805.5 702.0
      714.0 823.5
      594.0 913.5
   ])
   report: true
]

out: object [
	edges: none ; we are interested only in the edges
]

triangulate inp out

img: draw 1024x1024 [

   ; draw edges...
   line-width 2
   pen blue   line  :inp/points :out/edges

   ; draw input points
   line-width 0
   point-size 10
   fill red   point :inp/points
]

save %out/spiral.png img
