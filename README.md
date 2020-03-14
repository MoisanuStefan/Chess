# ChessServer
Concurrent server handles multiple chess games happening at the same time between any number of clients.</br>
There is no GUI as the target of the project was on establishing connections and managing processes.</br>
**Run Instructions:**</br>
- player id is required for keeping track of each score in the database</br>
- after an opponent is connected, the game begins and moves are requested in format of 4 ints:</br> **source_line source_column destination_line destination_column**</br>
Project made for college subject Computer Networks (FII, year 2)
