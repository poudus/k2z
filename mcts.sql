
create sequence k2s.mcts_sequo start 1;

create table k2s.mcts (
id int primary key,
depth int not null,
parent int not null,
sid int not null,
move char(2) not null,
code varchar(32) not null,
score numeric(10,1) not null,
visits int not null,
ratio numeric(6,4) not null
);

insert into k2s.mcts values (0, 0, -1, -1, '', '', 0.0, 0, 0.0);

create index idx_mcts_parent on k2s.mcts(parent);
