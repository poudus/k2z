
create table k2s.player (
id int primary key,
name varchar(16) not null,
win int not null,
loss int not null,
draw int not null,
rating numeric(6,2) not null,
lambda_decay numeric(6,2) not null,
opp_decay numeric(6,2) not null,
spread numeric(6,2) not null,
zeta numeric(6,2) not null,
max_moves int not null,
depth int not null
);

