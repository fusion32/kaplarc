-- database schema notes:
--	* no multiple worlds
--	* no account <-> player foreign constraints
--	* account characters are kept in a string separated by ';'s
--	  (using the PGSQL array type could also work but as libpq
--	  send a string like '{first, second, third}' for array types
--	  i figured it would be simpler to have a ';' delimited list
--	  as parsing it would be trivial)
--	* account premend is a date type because it's simpler to keep
--	  dates instead of timestamps which may vary and also having
--	  the granularity in days makes more sense



-- #######################################################################
-- PGSQL NOTES:
--
-- CREATE INDEX account_name_lookup ON accounts USING HASH(name);
-- adding a unique constraint will automatically create a unique B-tree index
-- adding a primary key will automatically create a unique B-tree index
-- #######################################################################

BEGIN;

CREATE TABLE accounts (
	name varchar(32) not null,
	password varchar(64) not null,
	premend date not null default '1970-01-01',
	charlist text not null default '',
	PRIMARY KEY (name)
);

CREATE TABLE players (
	id serial,
	name varchar(32) not null,
	PRIMARY KEY (id)
);
CREATE UNIQUE INDEX players_lower_name_key ON players (lower(name));

INSERT INTO players (name) VALUES
	('GameMaster'),
	('Player1'),
	('Player2');

INSERT INTO accounts (name, password, premend, charlist) VALUES
	('admin', 'admin', '2999-12-31', 'GameMaster'),
	('acctest', 'pwdtest', DEFAULT, 'Player1;Player2');

COMMIT;