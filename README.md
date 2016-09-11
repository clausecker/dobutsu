Dōbutsu shōgi
=============

This is a software that plays [Dōbutsu shōgi]&#x20;(動物将棋).  It works
by generating a table of all possible board configurations and is
capable of always choosing the best possible move.  The software is
unfinished, so far there is a program `bestmove` that can tell you what
the best move from a given board configuration is.  Eventually there is
going to be an interactive Dōbutsu shōgi AI with configurable strength.

Below you can find some explanations for the input and output formats
used by `bestmove`. All of this is subject to change.

How to build the software
=========================

Invoke `make` to build the software.  To run the software succesfully,
you also need the game database file `game.db`.  You can either generate
the file by running `make game.db` (this takes 10&nbsp;minutes on a fast
computer, 30&nbsp;minutes on a slower one) or download it from my
website by running `make fetch-gamedb`.  The software is written in
standard C with no other dependencies.

The bestmove program
====================

The `bestmove` program takes a position string and shows all possible
moves and how good they are.  Move quality is indicated by distance to
win/loss.  Here is an example:

    $ bestmove G/l-R/-e-/-C-/L--/Geg
    +---+
    |l R| ge
    | e |
    | C |
    |L  | G
    +---+

    Position is a win  in 11. Possible moves:
     1: Bl8  win  in  10: S/--R/le-/-C-/L--/Gge
     2: 7e9  win  in  16: S/l-e/---/-C-/L--/Ggec
     3: *g5  draw:        S/l-R/-e-/gC-/L--/Ge
     4: *eA  loss in  21: S/leR/-e-/-C-/L--/Gg
     5: *g1  loss in  19: S/l-R/-e-/-C-/Lg-/Ge
     6: *gA  loss in  19: S/lgR/-e-/-C-/L--/Ge
     7: 7e5  loss in  17: S/l-R/---/eC-/L--/Gge
     8: 7e3  loss in  17: S/l-R/---/-Ce/L--/Gge
     9: *e5  loss in  17: S/l-R/-e-/eC-/L--/Gg
    10: *e8  loss in  15: S/l-R/ee-/-C-/L--/Gg
    11: *e3  loss in  13: S/l-R/-e-/-Ce/L--/Gg
    12: *e6  loss in  13: S/l-R/-ee/-C-/L--/Gg
    13: *g6  loss in   9: S/l-R/-eg/-C-/L--/Ge
    14: *g0  loss in   7: S/l-R/-e-/-C-/L-g/Ge
    15: *e1  loss in   7: S/l-R/-e-/-C-/Le-/Gg
    16: *e0  loss in   7: S/l-R/-e-/-C-/L-e/Gg
    17: *g3  loss in   7: S/l-R/-e-/-Cg/L--/Ge
    18: *g8  loss in   3: S/l-R/ge-/-C-/L--/Ge
    19: BlA  loss in   1: S/-lR/-e-/-C-/L--/Gge

Board coordinates
=================

For simplicity, the program uses hexadecimal board coordinates with each
field having a number from 0 to B.  Beside the following diagram you can
see the conventional coordinates used on the commercial sets.

      ABC
     +---+
    1|BA9|
    2|876|
    3|543|
    4|210|
     +---+

Board visualization
===================

Capital letter means owned by sente, lower-case letter is owned by gote.  Pieces:

    L/l: lion
    E/e: elephant
    G/g: giraffe
    C/C: chick
    R/r: rooster

Pieces in hand are placed to the right of the board, like this:

    +---+
    | lg| c
    | e |
    |   |
    |GLE| C
    +---+

For example, the initial position is displayed as

    +---+
    |gle| 
    | c |
    | C |
    |ELG| 
    +---+

There are two positions that end in a stalemate because Sente cannot
move.  They look like this:

    +---+    +---+
    |lEG|    | EG|
    |EGL|    |EGL|
    | CC|    |lCC|
    |   |    |   |
    +---+    +---+

Position notation
=================

We can describe a game position by a string of the form

   T/BA9/876/543/210/H

where 0–B is what is on the board at the indicated square (uppercase for
Sente's pieces, lowercase for Gote's pieces, - for nothing), T is who's
turn it is (S for Sente, G for Gote) and H are the pieces in hand
(ownership being indicated by case).  If neither party has pieces in
hand, this can be indicated with a dash.

    S/gle/-c-/-C-/ELG/-

and this position with Gote having the right to move first:

    +---+
    |l R| eg
    | e |
    | C |
    |L  | G
    +---+

is encoded as

    G/l-R/-e-/-C-/L--/Geg

Move notation
=============

A move is denotes by a four character string of the form

    FPT+

    F: The square from which the piece moves or * if it is dropped.
    P: The piece that moves (one of CLEGR). If the move promotes, the piece
       name is what the piece was called before the promotion (i.e. C).
    T: The square the piece moves to.
    +: The character '+' if this move is a promoting move, a space if it
       isn't.

[Dōbutsu shōgi]: https://en.wikipedia.org/wiki/D%C5%8Dbutsu_sh%C5%8Dgi
