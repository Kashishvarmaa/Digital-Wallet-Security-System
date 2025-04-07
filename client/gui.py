import tkinter as tk
from tkinter import messagebox, ttk
from collections import defaultdict
import re
import pyperclip
from wallet_client import WalletClient  # Handles socket communication
import sqlite3
import os


client = WalletClient()  # Connects to server on start

class WalletApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Secure Wallet 💸")
        self.root.geometry("400x400")
        self.root.resizable(False, False)
        self.username = None  # To store logged in username
        self.login_screen()

    def login_screen(self):
        self.clear_window()

        tk.Label(self.root, text="Login or Signup", font=("Helvetica", 16)).pack(pady=10)

        tk.Label(self.root, text="Username").pack()
        self.username_entry = tk.Entry(self.root)
        self.username_entry.pack()

        tk.Label(self.root, text="Password").pack()
        self.password_entry = tk.Entry(self.root, show="*")
        self.password_entry.pack()

        tk.Button(self.root, text="Login", command=self.login).pack(pady=5)
        tk.Button(self.root, text="Signup", command=self.signup).pack()

    def login(self):
        user = self.username_entry.get()
        pwd = self.password_entry.get()
        response = client.send_command(f"LOGIN {user} {pwd}")
        if "successful" in response:
            self.username = user
            self.wallet_screen()
        else:
            messagebox.showerror("Login Failed", response)

    def signup(self):
        user = self.username_entry.get()
        pwd = self.password_entry.get()
        response = client.send_command(f"SIGNUP {user} {pwd}")
        messagebox.showinfo("Signup", response)

    def wallet_screen(self):
        self.clear_window()

        tk.Label(self.root, text=f"Welcome, {self.username}!", font=("Helvetica", 14)).pack(pady=10)

        tk.Button(self.root, text="Check Balance", command=self.check_balance).pack(pady=5)
        tk.Button(self.root, text="Transfer Money", command=self.transfer_screen).pack(pady=5)
        tk.Button(self.root, text="Transaction History", command=lambda: self.show_transaction_history(self.username)).pack(pady=5)

        if self.username == "admin":
            tk.Button(self.root, text="Admin: Show All Users", command=self.show_admin_all_users).pack(pady=5)
            tk.Button(self.root, text="📊 Admin Panel", command=self.show_admin_stats).pack(pady=5)

        tk.Button(self.root, text="Logout", command=self.login_screen).pack(pady=20)

    def check_balance(self):
        response = client.send_command(f"BALANCE {self.username}")
        messagebox.showinfo("Balance", response)

    def view_history(self):
        response = client.send_command(f"HISTORY {self.username}")
        messagebox.showinfo("History", response)

    def transfer_screen(self):
        self.clear_window()

        tk.Label(self.root, text="Send Money", font=("Helvetica", 16)).pack(pady=10)

        tk.Label(self.root, text="Recipient").pack()
        self.recipient_entry = tk.Entry(self.root)
        self.recipient_entry.pack()

        tk.Label(self.root, text="Amount").pack()
        self.amount_entry = tk.Entry(self.root)
        self.amount_entry.pack()

        tk.Button(self.root, text="Send", command=self.send_money).pack(pady=10)
        tk.Button(self.root, text="Back", command=self.wallet_screen).pack()

    def show_transaction_history(self, user_id):
        history_window = tk.Toplevel(self.root)
        history_window.title(f"Transaction History - {user_id}")
        history_window.geometry("600x400")
        history_window.configure(bg="#1e1e1e")

        # Scrollable text widget
        text_frame = tk.Frame(history_window, bg="#1e1e1e")
        text_frame.pack(fill="both", expand=True, padx=10, pady=10)

        scrollbar = tk.Scrollbar(text_frame)
        scrollbar.pack(side="right", fill="y")

        text_widget = tk.Text(
            text_frame,
            wrap="word",
            yscrollcommand=scrollbar.set,
            bg="#2c2c2c",
            fg="white",
            insertbackground="white",
            font=("Courier", 11)
        )
        text_widget.pack(fill="both", expand=True)
        scrollbar.config(command=text_widget.yview)

        # Fetch data
        conn = sqlite3.connect('/Users/kashishvarmaa/Documents/6 Sem/NS/Digital Secure Wallet System/server/wallet.db')
        c = conn.cursor()
        try:
            c.execute(
                "SELECT sender, receiver, amount, timestamp FROM transactions WHERE sender=? OR receiver=? ORDER BY timestamp DESC",
                (user_id, user_id)
            )
            transactions = c.fetchall()
        except sqlite3.OperationalError as e:
            text_widget.insert("end", f"Error fetching transactions: {e}")
            transactions = []
        conn.close()

        if not transactions:
            text_widget.insert("end", "No transactions found.\n")
        else:
            for txn in transactions:
                sender, receiver, amount, timestamp = txn
                entry = f"Sender: {sender}\nReceiver: {receiver}\nAmount: ₹{amount:.2f}\nTime: {timestamp}\n{'-'*50}\n"
                text_widget.insert("end", entry)



    def send_money(self):
        recipient = self.recipient_entry.get()
        amount = self.amount_entry.get()

        try:
            float_amount = float(amount)
            if float_amount > 1000:
                messagebox.showwarning("Limit Exceeded", "Max allowed: ₹1000")
                return
        except ValueError:
            messagebox.showerror("Invalid Input", "Enter a valid number.")
            return

        response = client.send_command(f"TRANSFER {recipient} {amount}")
        messagebox.showinfo("Transfer", response)
        self.wallet_screen()

    def clear_window(self):
        for widget in self.root.winfo_children():
            widget.destroy()

    def show_admin_stats(self):
        response = client.send_command(f"ADMIN_STATS {self.username}")
        messagebox.showinfo("Admin Stats", response)

    def parse_show_all_users(self, raw):
        users = []
        txns = {}
        lines = raw.strip().split('\n')

        current_user = None
        in_txn_block = False

        for line in lines:
            line = line.strip()
            if line.startswith("User:"):
                current_user = line.split(":", 1)[1].strip()
                users.append({"username": current_user})
                txns[current_user] = []
                in_txn_block = False  # Reset txn block
            elif line.startswith("Password:"):
                continue  # we don't need to store password
            elif line.startswith("Balance:"):
                balance_str = line.split("₹")[-1].strip()
                if users:
                    users[-1]["balance"] = float(balance_str)
            elif line.startswith("Transaction History:"):
                in_txn_block = True
            elif in_txn_block and line.startswith("From:"):
                parts = line.split("|")
                from_user = parts[0].split(":", 1)[1].strip()
                to_user = parts[1].split(":", 1)[1].strip()
                amount = float(parts[2].strip("₹ ").strip())
                time = parts[3].strip()
                txns[current_user].append({
                    "from": from_user,
                    "to": to_user,
                    "amount": amount,
                    "time": time
                })

        print("[DEBUG] Parsed users:", users)
        print("[DEBUG] Parsed txns:", txns)
        return users, txns

    def show_admin_all_users(self):
        raw = client.send_command("SHOW_ALL_USERS")  # no username
        users, txns = self.parse_show_all_users(raw)

        win = tk.Toplevel(self.root)
        win.title("👩‍💻 All Users Overview")
        win.geometry("900x600")

        # USERS TABLE
        ttk.Label(win, text="👥 Users", font=('Helvetica', 14, 'bold')).pack(pady=10)

        user_frame = ttk.Frame(win)
        user_frame.pack(fill='x', padx=10)

        user_tree = ttk.Treeview(user_frame, columns=("Username", "Balance"), show="headings", height=8)
        user_tree.heading("Username", text="Username")
        user_tree.heading("Balance", text="Balance (₹)")
        user_tree.column("Username", width=200)
        user_tree.column("Balance", width=150)

        user_scroll_y = ttk.Scrollbar(user_frame, orient="vertical", command=user_tree.yview)
        user_tree.configure(yscroll=user_scroll_y.set)
        user_tree.pack(side='left', fill='both', expand=True)
        user_scroll_y.pack(side='right', fill='y')

        for i, u in enumerate(users):
            tag = 'even' if i % 2 == 0 else 'odd'
            user_tree.insert("", tk.END, values=(u["username"], f"{u['balance']:.2f}"), tags=(tag,))
        user_tree.tag_configure('even', background='#1e1e1e', foreground='white')
        user_tree.tag_configure('odd', background='#000000', foreground='white')

        # TRANSACTIONS TABLE
        ttk.Label(win, text="📜 Transaction History (Select a user)", font=('Helvetica', 13)).pack(pady=10)

        txn_frame = ttk.Frame(win)
        txn_frame.pack(fill='both', padx=10, expand=True)

        txn_tree = ttk.Treeview(txn_frame, columns=("From", "To", "Amount", "Time"), show="headings", height=10)
        for col in ["From", "To", "Amount", "Time"]:
            txn_tree.heading(col, text=col)
            txn_tree.column(col, width=150 if col != "Time" else 300)

        txn_scroll_y = ttk.Scrollbar(txn_frame, orient="vertical", command=txn_tree.yview)
        txn_tree.configure(yscroll=txn_scroll_y.set)
        txn_tree.pack(side='left', fill='both', expand=True)
        txn_scroll_y.pack(side='right', fill='y')

        def on_user_select(event):
            txn_tree.delete(*txn_tree.get_children())
            selected = user_tree.selection()
            if selected:
                uname = user_tree.item(selected[0])["values"][0]
                print(f"[DEBUG] Selected username from Treeview: '{uname}'")
                print(f"[DEBUG] Available usernames in txns dict: {list(txns.keys())}")
                for i, tx in enumerate(txns.get(uname, [])):
                    color_tag = f"{'debit' if tx['from'] == uname else 'credit'}_{i % 2}"
                    txn_tree.insert("", tk.END, values=(
                        tx["from"], tx["to"], f"{tx['amount']:.2f}", tx["time"]
                    ), tags=(color_tag,))
                    txn_tree.tag_configure('debit_0', background="#ffe6e6", foreground="red")
                    txn_tree.tag_configure('debit_1', background="#fff2f2", foreground="red")
                    txn_tree.tag_configure('credit_0', background="#e6ffe6", foreground="green")
                    txn_tree.tag_configure('credit_1', background="#f2fff2", foreground="green")

        user_tree.bind("<<TreeviewSelect>>", on_user_select)

        def copy_to_clipboard():
            selected_user = user_tree.selection()
            if not selected_user:
                messagebox.showwarning("No User", "Please select a user to copy their transactions.")
                return

            uname = user_tree.item(selected_user[0])["values"][0]
            user_txns = txns.get(uname, [])
            if not user_txns:
                messagebox.showinfo("No Transactions", f"No transactions for {uname}")
                return

            text_data = f"Transactions for {uname}:\n"
            for tx in user_txns:
                text_data += f"From: {tx['from']} | To: {tx['to']} | ₹{tx['amount']:.2f} | {tx['time']}\n"

            pyperclip.copy(text_data.strip())
            messagebox.showinfo("Copied", "Transaction history copied to clipboard.")

        tk.Button(win, text="📋 Copy to Clipboard", command=copy_to_clipboard).pack(pady=10)

# --- Run the App ---
if __name__ == "__main__":
    root = tk.Tk()
    app = WalletApp(root)
    root.mainloop()