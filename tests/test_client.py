#!/usr/bin/env python3
"""
Simple test client for the Paged Key-Value Cache Server
Demonstrates the protocol and tests basic functionality.
"""

import socket
import time
import sys
import threading

class CacheClient:
    def __init__(self, host='localhost', port=8080):
        self.host = host
        self.port = port
        self.socket = None
    
    def connect(self):
        """Connect to the cache server"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            print(f"Connected to server at {self.host}:{self.port}")
            return True
        except Exception as e:
            print(f"Failed to connect: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from the server"""
        if self.socket:
            self.socket.close()
            self.socket = None
            print("Disconnected from server")
    
    def send_command(self, method, key, value=None):
        """Send a command to the server and return the response"""
        if not self.socket:
            return "ERROR: Not connected to server"
        
        # Build the message
        message = f"Method:{method}\r\nKey:{key}\r\n"
        if value is not None:
            message += f"Value:{value}\r\n"
        message += "\r\n"
        
        try:
            # Send the message
            self.socket.send(message.encode('utf-8'))
            
            # Receive the response
            response = self.socket.recv(4096).decode('utf-8')
            return response.strip()
        except Exception as e:
            return f"ERROR: Communication failed - {e}"
    
    def add(self, key, value):
        """Add a new key-value pair"""
        return self.send_command("ADD", key, value)
    
    def update(self, key, value):
        """Update an existing key-value pair"""
        return self.send_command("UPDATE", key, value)
    
    def get(self, key):
        """Get the value for a key"""
        return self.send_command("GET", key)
    
    def delete(self, key):
        """Delete a key-value pair"""
        return self.send_command("DELETE", key)

def run_basic_tests():
    """Run basic functionality tests"""
    print("=== Running Basic Tests ===")
    
    client = CacheClient()
    if not client.connect():
        return False
    
    try:
        # Test ADD
        print("\n1. Testing ADD operation...")
        response = client.add("test_key", "Hello World")
        print(f"ADD response: {response}")
        
        # Test GET
        print("\n2. Testing GET operation...")
        response = client.get("test_key")
        print(f"GET response: {response}")
        
        # Test UPDATE
        print("\n3. Testing UPDATE operation...")
        response = client.update("test_key", "Updated Value")
        print(f"UPDATE response: {response}")
        
        # Test GET after update
        print("\n4. Testing GET after UPDATE...")
        response = client.get("test_key")
        print(f"GET response: {response}")
        
        # Test DELETE
        print("\n5. Testing DELETE operation...")
        response = client.delete("test_key")
        print(f"DELETE response: {response}")
        
        # Test GET after delete
        print("\n6. Testing GET after DELETE...")
        response = client.get("test_key")
        print(f"GET response: {response}")
        
        # Test duplicate ADD
        print("\n7. Testing duplicate ADD...")
        client.add("dup_key", "value1")
        response = client.add("dup_key", "value2")
        print(f"Duplicate ADD response: {response}")
        
        # Clean up
        client.delete("dup_key")
        
        print("\n=== Basic Tests Completed ===")
        return True
        
    finally:
        client.disconnect()

def test_large_values():
    """Test with large values to exercise page allocation"""
    print("\n=== Testing Large Values ===")
    
    client = CacheClient()
    if not client.connect():
        return False
    
    try:
        # Test with different sized values
        sizes = [100, 1024, 10240, 40960, 100000]  # Up to ~100KB
        
        for size in sizes:
            print(f"\nTesting value size: {size} bytes")
            large_value = "x" * size
            
            response = client.add(f"large_key_{size}", large_value)
            print(f"ADD response: {response}")
            
            if "OK" in response:
                get_response = client.get(f"large_key_{size}")
                if f"Value={'x' * size}" in get_response:
                    print("✓ Large value stored and retrieved correctly")
                else:
                    print("✗ Large value retrieval failed")
                
                # Clean up
                client.delete(f"large_key_{size}")
            
        print("\n=== Large Value Tests Completed ===")
        return True
        
    finally:
        client.disconnect()

def test_concurrent_clients():
    """Test multiple concurrent clients"""
    print("\n=== Testing Concurrent Clients ===")
    
    def client_worker(client_id, num_operations=10):
        """Worker function for concurrent client testing"""
        client = CacheClient()
        if not client.connect():
            print(f"Client {client_id} failed to connect")
            return
        
        try:
            for i in range(num_operations):
                key = f"client_{client_id}_key_{i}"
                value = f"value_from_client_{client_id}_{i}"
                
                # Add
                response = client.add(key, value)
                if "OK" not in response:
                    print(f"Client {client_id}: ADD failed for {key}")
                
                # Get
                response = client.get(key)
                if f"Value={value}" not in response:
                    print(f"Client {client_id}: GET failed for {key}")
                
                # Update
                new_value = value + "_updated"
                response = client.update(key, new_value)
                if "OK" not in response:
                    print(f"Client {client_id}: UPDATE failed for {key}")
                
                # Delete
                response = client.delete(key)
                if "OK" not in response:
                    print(f"Client {client_id}: DELETE failed for {key}")
                
                time.sleep(0.01)  # Small delay
            
            print(f"Client {client_id} completed {num_operations} operations")
            
        finally:
            client.disconnect()
    
    # Create multiple concurrent clients
    threads = []
    num_clients = 5
    
    for i in range(num_clients):
        thread = threading.Thread(target=client_worker, args=(i,))
        threads.append(thread)
        thread.start()
    
    # Wait for all clients to complete
    for thread in threads:
        thread.join()
    
    print(f"\n=== Concurrent Client Tests Completed ({num_clients} clients) ===")
    return True

def interactive_mode():
    """Interactive mode for manual testing"""
    print("\n=== Interactive Mode ===")
    print("Commands: add <key> <value>, get <key>, update <key> <value>, delete <key>, quit")
    
    client = CacheClient()
    if not client.connect():
        return
    
    try:
        while True:
            try:
                cmd_input = input("\ncache> ").strip().split()
                if not cmd_input:
                    continue
                
                cmd = cmd_input[0].lower()
                
                if cmd == "quit" or cmd == "exit":
                    break
                elif cmd == "add" and len(cmd_input) >= 3:
                    key = cmd_input[1]
                    value = " ".join(cmd_input[2:])
                    response = client.add(key, value)
                    print(response)
                elif cmd == "get" and len(cmd_input) == 2:
                    key = cmd_input[1]
                    response = client.get(key)
                    print(response)
                elif cmd == "update" and len(cmd_input) >= 3:
                    key = cmd_input[1]
                    value = " ".join(cmd_input[2:])
                    response = client.update(key, value)
                    print(response)
                elif cmd == "delete" and len(cmd_input) == 2:
                    key = cmd_input[1]
                    response = client.delete(key)
                    print(response)
                else:
                    print("Invalid command. Use: add <key> <value>, get <key>, update <key> <value>, delete <key>, quit")
            
            except KeyboardInterrupt:
                break
            except Exception as e:
                print(f"Error: {e}")
    
    finally:
        client.disconnect()

def main():
    if len(sys.argv) > 1 and sys.argv[1] == "interactive":
        interactive_mode()
    else:
        print("Paged Key-Value Cache Server Test Client")
        print("Make sure the server is running before starting tests...")
        time.sleep(1)
        
        success = True
        success &= run_basic_tests()
        time.sleep(1)
        success &= test_large_values()
        time.sleep(1)
        success &= test_concurrent_clients()
        
        if success:
            print("\n🎉 All tests completed successfully!")
        else:
            print("\n❌ Some tests failed.")
        
        print("\nTo run in interactive mode: python3 test_client.py interactive")

if __name__ == "__main__":
    main()