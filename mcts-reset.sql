
delete from k2s.mcts where id > 99;
update k2s.mcts set visits = 0, score = 0.0, ratio = 0.0;

alter sequence k2s.mcts_sequo restart with 100;

