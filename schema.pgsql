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

-- players consistency test:
-- SELECT COUNT(*) FROM players RIGHT JOIN accounts ON players.name = ANY(accounts.charlist);
-- SELECT COUNT(*) FROM players;
-- if count_a != count_b then theres a consistency problem

-- accounts consistency test:
-- SELECT COUNT(*) FROM accounts RIGHT JOIN players ON players.name = ANY(accounts.charlist);
-- SELECT SUM(ARRAY_LENGTH(accounts.charlist, 1)) FROM accounts;
-- if count_a != count_b then theres a consistency problem

-- overall consistency:
-- best is to use foreign keys (reduced complexity)


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
	charlist text array,
	PRIMARY KEY (name)
);

CREATE TABLE players (
	id serial,
	name varchar(32) not null,
	PRIMARY KEY (id)
);
CREATE UNIQUE INDEX player_name_upper_index ON players (UPPER(name));

INSERT INTO accounts (name, password, premend, charlist) VALUES
	('admin', 'admin', '2999-12-31', ARRAY['GameMaster']),
	('acctest', 'pwdtest', DEFAULT, ARRAY['Player1', 'Player2']);

INSERT INTO players (name) VALUES
	('GameMaster'),
	('Player1'),
	('Player2');

COMMIT;
