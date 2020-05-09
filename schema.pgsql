-- database schema notes:
--  * no multiple worlds
--  * foreign keys reduce parsing complexity
--    while keeping data consistency

-- #######################################################################
-- PGSQL NOTES:
-- adding a unique constraint will automatically create a unique B-tree index
-- adding a primary key will automatically create a unique B-tree index
-- #######################################################################

BEGIN;

CREATE TABLE accounts (
	account_id serial,
	name varchar(32) not null,
	password varchar(64) not null,
	premend date not null default '1970-01-01',
	PRIMARY KEY (account_id)
);
CREATE UNIQUE INDEX account_name_lower_index ON accounts (lower(name));

CREATE TABLE players (
	player_id serial,
	account_id int not null,
	name varchar(32) not null,
	PRIMARY KEY (player_id),
	FOREIGN KEY (account_id) REFERENCES accounts
);
CREATE UNIQUE INDEX player_name_lower_index ON players (lower(name));
CREATE INDEX player_account_index ON players (account_id);

INSERT INTO accounts (name, password, premend) VALUES
	('admin', 'admin', '2999-12-31'),
	('acctest', 'pwdtest', DEFAULT);

INSERT INTO players (account_id, name) VALUES
	(1, 'GameMaster'),
	(2, 'Player1'),
	(2, 'Player2');

COMMIT;
