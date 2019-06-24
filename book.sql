
create sequence k2s.book_sequo start 100;

create table k2s.book (
id int primary key,
depth int not null,
parent int not null,
move char(2) not null,
code varchar(32) not null,
eval numeric(6,2) not null,
deep_eval numeric(6,2) not null
);

insert into k2s.book values (0, 0, -1, '', '', 0.0, 0.0);

insert into k2s.book values (1, 1, 0, 'CC', 'CC', 0.0, 0.0);
insert into k2s.book values (2, 1, 0, 'DC', 'DC', 0.0, 0.0);
insert into k2s.book values (3, 1, 0, 'EC', 'EC', 0.0, 0.0);
insert into k2s.book values (4, 1, 0, 'FC', 'FC', 0.0, 0.0);

insert into k2s.book values (5, 1, 0, 'CD', 'CD', 0.0, 0.0);
insert into k2s.book values (6, 1, 0, 'DD', 'DD', 0.0, 0.0);
insert into k2s.book values (7, 1, 0, 'ED', 'ED', 0.0, 0.0);
insert into k2s.book values (8, 1, 0, 'FD', 'FD', 0.0, 0.0);

insert into k2s.book values (9, 1, 0, 'CE', 'CE', 0.0, 0.0);
insert into k2s.book values (10, 1, 0, 'DE', 'DE', 0.0, 0.0);
insert into k2s.book values (11, 1, 0, 'EE', 'EE', 0.0, 0.0);
insert into k2s.book values (12, 1, 0, 'FE', 'FE', 0.0, 0.0);

insert into k2s.book values (13, 1, 0, 'CF', 'CF', 0.0, 0.0);
insert into k2s.book values (14, 1, 0, 'DF', 'DF', 0.0, 0.0);
insert into k2s.book values (15, 1, 0, 'EF', 'EF', 0.0, 0.0);
insert into k2s.book values (16, 1, 0, 'FF', 'FF', 0.0, 0.0);

create index idx_book_parent on k2s.book(parent);
create index idx_code_parent on k2s.book(code);

