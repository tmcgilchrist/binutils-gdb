(* Basic OCaml DWARF 5 test - Phase 1 features
   Tests variants, records, arrays, and tuples *)

(* Simple variant type *)
type color = Red | Green | Blue of int

(* Variant with multiple fields *)
type shape =
  | Circle of float
  | Rectangle of int * int
  | Point

(* Record type *)
type person = { name: string; age: int }

(* Record with multiple fields *)
type coordinate = { x: int; y: int; z: int }

(* Functions to test - marked to prevent inlining *)
let[@inline never] test_variants () =
  let v1 = Red in
  let v2 = Green in
  let v3 = Blue 42 in
  let s1 = Point in
  let s2 = Circle 3.14 in
  let s3 = Rectangle (10, 20) in
  (v1, v2, v3, s1, s2, s3)  (* Breakpoint 1 *)

let[@inline never] test_records () =
  let p1 = { name = "Alice"; age = 30 } in
  let p2 = { name = "Bob"; age = 25 } in
  let c1 = { x = 1; y = 2; z = 3 } in
  let c2 = { x = 0; y = 0; z = 0 } in
  (p1, p2, c1, c2)  (* Breakpoint 2 *)

let[@inline never] test_arrays () =
  let arr1 = [| 1; 2; 3 |] in
  let arr2 = [| |] in
  (arr1, arr2)  (* Breakpoint 3 *)

let[@inline never] test_tuples () =
  let t1 = (1, 2) in
  let t2 = (1, 2, 3) in
  let t3 = ("hello", 42, 3.14) in
  (t1, t2, t3)  (* Breakpoint 4 *)

(* Main entry point *)
let () =
  let _ = test_variants () in
  let _ = test_records () in
  let _ = test_arrays () in
  let _ = test_tuples () in
  ()
