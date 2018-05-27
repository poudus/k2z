
create schema k2s;

create sequence k2s.game_sequo start 1;

create table k2s.game (
id int primary key,
ts bigint not null,
hp int not null,
vp int not null,
moves varchar(128) not null,
winner char(1) not null,
reason char(1),
duration int not null,
length int not null,
ht int not null,
vt int not null
);

create table k2s.player (
id int primary key,
name varchar(16) not null,
win int not null,
loss int not null,
draw int not null,
rating numeric(6,2) not null,
lambda_decay numeric(6,2) not null,
opp_decay numeric(6,2) not null,
max_moves int not null,
depth int not null
);

