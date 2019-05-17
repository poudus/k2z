
create sequence k2s.mcts_sequo start 100;

create table k2s.mcts (
id int primary key,
depth int not null,
parent int not null,
sid int not null,
move char(2) not null,
code varchar(32) not null,
score numeric(12,2) not null,
visits int not null,
ratio numeric(6,4) not null
);

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

create index idx_mcts_parent on k2s.mcts(parent);
create index idx_mcts_code on k2s.mcts(code);
