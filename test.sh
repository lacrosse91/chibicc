#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./chibicc "$input" > tmp.s
  gcc -static -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 0
assert 42 42
assert 21 '5+20-4'
assert 15 '10 + 8 - 3'
assert 6 '2 * 3'
assert 18 '2 + 4 * 4'
assert 13 '12 / 4 + 5 * 2'
assert 14 '2 * (3 + 4)'
assert 15 '1 + 2 + 3 + 4 + 5'
assert 6 '+6'
assert 4 '-8 + 12'

echo OK
