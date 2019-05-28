
delete from k2s.mcts where id > 99;
update k2s.mcts set visits = 0, score = 0.0, ratio = 0.0;

alter sequence k2s.mcts_sequo restart with 100;

insert into k2s.mcts values (0, 0, -1, -1, '', '', 0.0, 0, 0.0);

insert into k2s.mcts values (1, 1, 0, 24, 'CC', 'CC', 0.0, 0, 0.0);
insert into k2s.mcts values (2, 1, 0, 25, 'DC', 'DC', 0.0, 0, 0.0);
insert into k2s.mcts values (3, 1, 0, 26, 'EC', 'EC', 0.0, 0, 0.0);
insert into k2s.mcts values (4, 1, 0, 27, 'FC', 'FC', 0.0, 0, 0.0);

insert into k2s.mcts values (5, 1, 0, 36, 'CD', 'CD', 0.0, 0, 0.0);
insert into k2s.mcts values (6, 1, 0, 37, 'DD', 'DD', 0.0, 0, 0.0);
insert into k2s.mcts values (7, 1, 0, 38, 'ED', 'ED', 0.0, 0, 0.0);
insert into k2s.mcts values (8, 1, 0, 39, 'FD', 'FD', 0.0, 0, 0.0);

insert into k2s.mcts values (9, 1, 0, 48, 'CE', 'CE', 0.0, 0, 0.0);
insert into k2s.mcts values (10, 1, 0, 49, 'DE', 'DE', 0.0, 0, 0.0);
insert into k2s.mcts values (11, 1, 0, 50, 'EE', 'EE', 0.0, 0, 0.0);
insert into k2s.mcts values (12, 1, 0, 51, 'FE', 'FE', 0.0, 0, 0.0);

insert into k2s.mcts values (13, 1, 0, 60, 'CF', 'CF', 0.0, 0, 0.0);
insert into k2s.mcts values (14, 1, 0, 61, 'DF', 'DF', 0.0, 0, 0.0);
insert into k2s.mcts values (15, 1, 0, 62, 'EF', 'EF', 0.0, 0, 0.0);
insert into k2s.mcts values (16, 1, 0, 63, 'FF', 'FF', 0.0, 0, 0.0);
