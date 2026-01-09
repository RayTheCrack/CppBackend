#!/bin/bash
# tcp_concurrent_test.sh
# 测试参数配置
SERVER_IP="192.168.182.128"
SERVER_PORT="9999"
CONCURRENT=10000  # 并发数
TOTAL_REQUESTS=10000  # 总请求数
REQUEST_PER_PROCESS=$((TOTAL_REQUESTS / CONCURRENT))

# 统计变量
SUCCESS=0
FAIL=0
START_TIME=$(date +%s%3N)  # 毫秒级开始时间

# 单个进程的测试逻辑
test_request() {
    local pid=$1
    local req_count=0
    local fail_count=0
    
    for ((i=0; i<REQUEST_PER_PROCESS; i++)); do
        # 使用nc发送TCP请求并接收响应
        RESP=$(echo "Test request from process $pid: $i" | nc -w 1 $SERVER_IP $SERVER_PORT 2>/dev/null)
        if [ $? -eq 0 ] && [ -n "$RESP" ]; then
            ((req_count++))
        else
            ((fail_count++))
        fi
    done
    
    # 更新全局统计（用文件避免竞争）
    echo $req_count >> /tmp/success.tmp
    echo $fail_count >> /tmp/fail.tmp
}

# 初始化统计文件
rm -f /tmp/success.tmp /tmp/fail.tmp
touch /tmp/success.tmp /tmp/fail.tmp

# 启动并发进程
echo "开始测试：并发数=$CONCURRENT，总请求数=$TOTAL_REQUESTS"
for ((i=0; i<CONCURRENT; i++)); do
    test_request $i &
done

# 等待所有进程完成
wait

# 汇总统计结果
SUCCESS=$(cat /tmp/success.tmp | awk '{sum+=$1} END{print sum}')
FAIL=$(cat /tmp/fail.tmp | awk '{sum+=$1} END{print sum}')
END_TIME=$(date +%s%3N)
DURATION=$((END_TIME - START_TIME))

# 输出测试结果
echo -e "\n=== 测试结果 ==="
echo "总耗时：$DURATION 毫秒"
echo "成功请求数：$SUCCESS"
echo "失败请求数：$FAIL"
echo "请求成功率：$((SUCCESS*100/(SUCCESS+FAIL)))%"
echo "QPS（每秒请求数）：$((SUCCESS*1000/DURATION))"

# 清理临时文件
rm -f /tmp/success.tmp /tmp/fail.tmp