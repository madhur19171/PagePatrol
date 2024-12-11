import sqlite3
import random
import string

# Database file
db_file = "people.db"

# Connect to the SQLite database
conn = sqlite3.connect(db_file)
cursor = conn.cursor()

# Helper functions
def random_name():
    # Generate a random first name
    first_name = ''.join(random.choices(string.ascii_uppercase + string.ascii_lowercase, k=random.randint(5, 15)))
    return first_name

def random_content():
    # Generate a random first name
    content = ''.join(random.choices(string.ascii_uppercase + string.ascii_lowercase, k=random.randint(20, 200)))
    return content

def random_email():
    # Generate a random username and choose a domain from the list
    domains = ["example.com", "test.com", "mail.com", "demo.org"]
    username = ''.join(random.choices(string.ascii_lowercase + string.digits, k=random.randint(5, 12)))
    return f"{username}@{random.choice(domains)}"

# Function to insert fake data
def insert_fake_users(n):
    for _ in range(n):
        name = random_name()
        email = random_email()
        try:
            cursor.execute("INSERT INTO users (name, email) VALUES (?, ?)", (name, email))
        except sqlite3.IntegrityError as e:
            print(f"Error inserting data: {e}")
    conn.commit()

def insert_fake_tweets(n):
    for _ in range(n):
        content = random_content()
        try:
            cursor.execute("INSERT INTO tweets (content) VALUES (?)", (content,))
        except sqlite3.IntegrityError as e:
            print(f"Error inserting data: {e}")
    conn.commit()

# num_users = 10_000
# insert_fake_users(num_users)
# print(f"Inserted {num_users} fake user(s) into the 'users' table.")

num_tweets = 1000
insert_fake_tweets(num_tweets)
print(f"Inserted {num_tweets} fake tweets(s) into the 'tweets' table.")

# Close the connection
conn.close()