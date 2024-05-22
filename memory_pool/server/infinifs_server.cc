

#include "infinifs_server.h"

#include <stdlib.h>
#include <unistd.h>

#include <thread>

#include "util/json_config.h"

void Server::AllocMem() {
  RDMA_LOG(INFO) << "Start allocating memory...";
  if (use_pm) {
    pm_file_fd = open(pm_file.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (pm_file_fd < 0) {
      printf("open file failed, %s\n", strerror(errno));
    }
    // 0x80003 = MAP_SHARED_VALIDATE | MAP_SYNC. Some old kernel does not have
    // linux/mman.h: MAP_SHARED_VALIDATE, MAP_SYNC
    hash_buffer = (char*)mmap(0, hash_buf_size, PROT_READ | PROT_WRITE, 0x80003,
                              pm_file_fd, 0);
    assert(hash_buffer);
    RDMA_LOG(INFO) << "Alloc PM data region success!";
  } else {
    hash_buffer = (char*)malloc(hash_buf_size);
    assert(hash_buffer);
    RDMA_LOG(INFO) << "Alloc DRAM data region success!";
  }
  offset_t reserve_start =
      hash_buf_size *
      0.75;  // reserve 1/4 for hash conflict in case of full bucket
  hash_reserve_buffer = hash_buffer + reserve_start;
  log_buffer = (char*)malloc(log_buf_size);
  assert(log_buffer);
}

void Server::InitMem() {
  RDMA_LOG(INFO) << "Start initializing memory...";
  memset(hash_buffer, 0, hash_buf_size);
  memset(log_buffer, 0, log_buf_size);
  RDMA_LOG(INFO) << "Initialize memory success!";
}

void Server::InitRDMA() {
  /************************************* RDMA+PM Initialization
   * ***************************************/
  RDMA_LOG(INFO) << "Start initializing RDMA...";
  rdma_ctrl = std::make_shared<RdmaCtrl>(server_node_id, local_port);
  RdmaCtrl::DevIdx idx{.dev_id = 0,
                       .port_id = 1};  // using the first RNIC's first port
  rdma_ctrl->open_thread_local_device(idx);
  RDMA_ASSERT(rdma_ctrl->register_memory(SERVER_HASH_BUFF_ID, hash_buffer,
                                         hash_buf_size,
                                         rdma_ctrl->get_device()) == true);
  RDMA_ASSERT(rdma_ctrl->register_memory(SERVER_LOG_BUFF_ID, log_buffer,
                                         log_buf_size,
                                         rdma_ctrl->get_device()) == true);
  RDMA_LOG(INFO) << "Register memory success!";
}

// All servers need to load data
void Server::LoadData(node_id_t machine_id,
                      node_id_t machine_num,  // number of memory nodes
                      std::string& workload) {
  /************************************* Load Data
   * ***************************************/
  RDMA_LOG(INFO) << "Start loading database data...";
  // Init tables
  MemStoreAllocParam mem_store_alloc_param(hash_buffer, hash_buffer, 0,
                                           hash_reserve_buffer);
  MemStoreReserveParam mem_store_reserve_param(hash_reserve_buffer, 0,
                                               hash_buffer + hash_buf_size);
  if (workload == "MICRO") {
    micro_server = new MICRO();
    micro_server->LoadTable(machine_id, machine_num, &mem_store_alloc_param,
                            &mem_store_reserve_param);
  }
  RDMA_LOG(INFO) << "Loading table successfully!";
}

void Server::CleanTable() {}

void Server::CleanQP() { rdma_ctrl->destroy_rc_qp(); }

void Server::SendMeta(node_id_t machine_id, std::string& workload,
                      size_t compute_node_num) {
  // Prepare hash meta
  char* hash_meta_buffer = nullptr;
  size_t total_meta_size = 0;
  PrepareHashMeta(machine_id, workload, &hash_meta_buffer, total_meta_size);
  assert(hash_meta_buffer != nullptr);
  assert(total_meta_size != 0);

  // Send memory store meta to all the compute nodes via TCP
  for (size_t index = 0; index < compute_node_num; index++) {
    SendHashMeta(hash_meta_buffer, total_meta_size);
  }
  free(hash_meta_buffer);
}

void Server::PrepareHashMeta(node_id_t machine_id, std::string& workload,
                             char** hash_meta_buffer, size_t& total_meta_size) {

}

void Server::SendHashMeta(char* hash_meta_buffer, size_t& total_meta_size) {
  //> Using TCP to send hash meta
  /* --------------- Initialize socket ---------------- */
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port =
      htons(local_meta_port);  // change host little endian to big endian
  server_addr.sin_addr.s_addr =
      htonl(INADDR_ANY);  // change host "0.0.0.0" to big endian
  int listen_socket = socket(AF_INET, SOCK_STREAM, 0);

  // The port can be used immediately after restart
  int on = 1;
  setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  if (listen_socket < 0) {
    RDMA_LOG(ERROR) << "Server creates socket error: " << strerror(errno);
    close(listen_socket);
    return;
  }
  RDMA_LOG(INFO) << "Server creates socket success";
  if (bind(listen_socket, (const struct sockaddr*)&server_addr,
           sizeof(server_addr)) < 0) {
    RDMA_LOG(ERROR) << "Server binds socket error: " << strerror(errno);
    close(listen_socket);
    return;
  }
  RDMA_LOG(INFO) << "Server binds socket success";
  int max_listen_num = 10;
  if (listen(listen_socket, max_listen_num) < 0) {
    RDMA_LOG(ERROR) << "Server listens error: " << strerror(errno);
    close(listen_socket);
    return;
  }
  RDMA_LOG(INFO) << "Server listens success";
  int from_client_socket = accept(listen_socket, NULL, NULL);
  // int from_client_socket = accept(listen_socket, (struct sockaddr*)
  // &client_addr, &client_socket_length);
  if (from_client_socket < 0) {
    RDMA_LOG(ERROR) << "Server accepts error: " << strerror(errno);
    close(from_client_socket);
    close(listen_socket);
    return;
  }
  RDMA_LOG(INFO) << "Server accepts success";

  /* --------------- Sending hash metadata ----------------- */
  auto retlen = send(from_client_socket, hash_meta_buffer, total_meta_size, 0);
  if (retlen < 0) {
    RDMA_LOG(ERROR) << "Server sends hash meta error: " << strerror(errno);
    close(from_client_socket);
    close(listen_socket);
    return;
  }
  RDMA_LOG(INFO) << "Server sends hash meta success";
  size_t recv_ack_size = 100;
  char* recv_buf = (char*)malloc(recv_ack_size);
  recv(from_client_socket, recv_buf, recv_ack_size, 0);
  if (strcmp(recv_buf, "[ACK]hash_meta_received_from_client") != 0) {
    std::string ack(recv_buf);
    RDMA_LOG(ERROR) << "Client receives hash meta error. Received ack is: "
                    << ack;
  }

  free(recv_buf);
  close(from_client_socket);
  close(listen_socket);
}

bool Server::Run() {
  // init threads to response

  while (true) {
    char ch;
    scanf("%c", &ch);
    if (ch == 'q') {
      return false;
    } else if (ch == 'c') {
      return true;
    } else {
      printf("Type c to run another round, type q if you want to exit :)\n");
    }
    usleep(2000);
  }
}

int main(int argc, char* argv[]) {
  // Configure of this server
  std::string config_filepath = "memory_node_config.json";
  auto json_config = JsonConfig::load_file(config_filepath);

  auto local_node = json_config.get("local_memory_node");
  node_id_t machine_num = (node_id_t)local_node.get("machine_num").get_int64();
  node_id_t machine_id = (node_id_t)local_node.get("machine_id").get_int64();
  assert(machine_id >= 0 && machine_id < machine_num);
  int local_port = (int)local_node.get("local_port").get_int64();
  int local_meta_port = (int)local_node.get("local_meta_port").get_int64();
  int use_pm = (int)local_node.get("use_pm").get_int64();
  std::string pm_root = local_node.get("pm_root").get_str();
  std::string workload = local_node.get("workload").get_str();
  auto mem_size_GB = local_node.get("mem_size_GB").get_uint64();
  auto log_buf_size_GB = local_node.get("log_buf_size_GB").get_uint64();

  auto compute_nodes = json_config.get("remote_compute_nodes");
  auto compute_node_ips = compute_nodes.get("compute_node_ips");  // Array
  size_t compute_node_num = compute_node_ips.size();

  // std::string pm_file = pm_root + "pm_node" + std::to_string(machine_id); //
  // Use fsdax
  std::string pm_file = pm_root;  // Use devdax
  size_t mem_size = (size_t)1024 * 1024 * 1024 * mem_size_GB;
  size_t hash_buf_size = mem_size;  // Currently, we support the hash structure
  size_t log_buf_size = (size_t)1024 * 1024 * 1024 * log_buf_size_GB;

  auto server = std::make_shared<Server>(
      machine_id, local_port, local_meta_port, hash_buf_size, log_buf_size,
      use_pm, pm_file, mem_size);
  server->AllocMem();
  server->InitMem();
  server->LoadData(machine_id, machine_num, workload);
  server->SendMeta(machine_id, workload, compute_node_num);
  server->InitRDMA();
  bool run_next_round = server->Run();

  // Continue to run the next round. RDMA does not need to be inited twice
  while (run_next_round) {
    server->InitMem();
    server->CleanTable();
    server->CleanQP();
    server->LoadData(machine_id, machine_num, workload);
    server->SendMeta(machine_id, workload, compute_node_num);
    run_next_round = server->Run();
  }

  // Stat the cpu utilization
  auto pid = getpid();
  std::string copy_cmd = "cp /proc/" + std::to_string(pid) + "/stat ./";
  system(copy_cmd.c_str());

  copy_cmd = "cp /proc/uptime ./";
  system(copy_cmd.c_str());
  return 0;
}