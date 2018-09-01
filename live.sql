
create table k2s.live (
channel int primary key,
ts bigint not null,
hp int not null,
vp int not null,
hpid int not null,
vpid int not null,
moves varchar(128) not null,
signature varchar(256) not null,
trait char(1) not null,
last_move char(2),
length int not null,
winner char(1) not null,
reason char(1) not null
);

