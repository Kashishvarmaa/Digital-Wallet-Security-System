import socket

SERVER_IP = "127.0.0.1"
PORT = 8080

logged_in_user = None

def send_request(request):
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.connect((SERVER_IP, PORT))
    client.send(request.encode())
    response = client.recv(4096).decode()  # Increased buffer for large responses like SHOW_ALL_USERS
    print("Server:", response)
    client.close()
    return response  # Return to parse login result

print("Welcome to Secure Wallet!")

while True:
    print("\nOptions:")
    print("1. Signup")
    print("2. Login")
    print("3. Check Balance")
    print("4. Transfer Money")
    if logged_in_user:
        print("5. Transaction History")
        print("6. Logout")
        print("7. Exit")
    else:
        print("5. Exit")
    print("9. View All Users (Admin Only)")

    choice = input("Enter choice: ")

    if choice == "1":
        username = input("Enter new username: ")
        password = input("Enter password: ")
        send_request(f"SIGNUP {username} {password}")

    

    elif choice == "2":
        if logged_in_user:
            print(f"Already logged in as {logged_in_user}. Logout first to login again.")
        else:
            username = input("Enter username: ")
            password = input("Enter password: ")
            response = send_request(f"LOGIN {username} {password}")
            if "successful" in response.lower():
                logged_in_user = username
            else:
                print("Login failed. Please try again.")

    elif choice == "3":
        if not logged_in_user:
            print("Please login first.")
        else:
            send_request(f"BALANCE {logged_in_user}")

    elif choice == "4":
        if not logged_in_user:
            print("Please login first.")
        else:
            receiver = input("Enter recipient's username: ")
            try:
                amount = float(input("Enter amount to transfer: "))
                send_request(f"TRANSFER {logged_in_user} {receiver} {amount}")
            except ValueError:
                print("Invalid amount! Please enter a number.")

    elif choice == "5":
        if logged_in_user:
            send_request(f"HISTORY {logged_in_user}")
        else:
            print("Goodbye!")
            break

    elif choice == "6" and logged_in_user:
        print(f"Logged out from {logged_in_user}")
        logged_in_user = None

    elif choice == "7" and logged_in_user:
        print("Goodbye!")
        break

    elif choice == "9":
        if logged_in_user:
            send_request(f"SHOW_ALL_USERS {logged_in_user}")
        else:
            print("Please login first.")

    else:
        print("Invalid choice! Please enter a valid option.")

    if not logged_in_user:  # Ask to continue only when logged out
        cont = input("\nDo you want to continue? (yes/no): ").lower()
        if cont != "yes":
            print("Goodbye!")
            break