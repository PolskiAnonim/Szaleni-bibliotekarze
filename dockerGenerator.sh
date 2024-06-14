#!/bin/bash
if [ "$#" -lt 5 ]; then
    echo "Usage: $0 <number_of_containers> <path_to_program> <arg1> <arg2> ... <argN>"
    echo "Example: $0 4 /path/to/bibliotekarze 3 2 2"
    exit 1
fi

MACHINE_COUNT=$1
PROGRAM_PATH=$2
shift 2
PROGRAM_ARGS="$@"
IMAGE_NAME="opensuse-openmpi"

cat <<EOF > Dockerfile
FROM opensuse/leap

RUN zypper --non-interactive ref && \
    zypper --non-interactive install gcc gcc-c++ cmake openmpi4 openmpi4-devel make openssh && \
    groupadd mpiuser && \
    useradd -ms /bin/bash -g mpiuser mpiuser && \
    echo "mpiuser:password" | chpasswd && \
    mkdir /home/mpiuser/.ssh && \
    ssh-keygen -t rsa -N "" -f /home/mpiuser/.ssh/id_rsa && \
    cat /home/mpiuser/.ssh/id_rsa.pub >> /home/mpiuser/.ssh/authorized_keys && \
    chown -R mpiuser:mpiuser /home/mpiuser/.ssh && \
    chmod 600 /home/mpiuser/.ssh/authorized_keys && \
    sed -i 's/#PermitRootLogin.*/PermitRootLogin yes/' /etc/ssh/sshd_config && \
    sed -i 's/PasswordAuthentication no/PasswordAuthentication yes/' /etc/ssh/sshd_config && \
    sed -i 's/#PermitEmptyPasswords no/PermitEmptyPasswords yes/' /etc/ssh/sshd_config && \
    sed -i 's/#UsePAM yes/UsePAM no/' /etc/ssh/sshd_config && \
    echo "Host *\n   StrictHostKeyChecking no\n   UserKnownHostsFile /dev/null" > /home/mpiuser/.ssh/config && \
    chown mpiuser:mpiuser /home/mpiuser/.ssh/config && \
    ssh-keygen -A

ENV PATH="/usr/lib64/mpi/gcc/openmpi4/bin:\$PATH"
ENV LD_LIBRARY_PATH="/usr/lib64/mpi/gcc/openmpi4/lib64:\$LD_LIBRARY_PATH"

EXPOSE 22
CMD ["/usr/sbin/sshd", "-D"]
EOF

docker build -t $IMAGE_NAME .

docker network create mpi-network

declare -A IP_ADDRESSES

for i in $(seq 1 $MACHINE_COUNT)
do
    docker run -t -d --name node$i --network mpi-network $IMAGE_NAME
    IP_ADDRESSES[node$i]=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' node$i)
done

for i in $(seq 1 $MACHINE_COUNT)
do
    for j in $(seq 1 $MACHINE_COUNT)
    do
        if [ $i -ne $j ]; then
            docker exec node$i sh -c "echo '${IP_ADDRESSES[node$j]} node$j' >> /etc/hosts"
        fi
    done
done

for i in $(seq 2 $MACHINE_COUNT)
do
    docker cp node1:/home/mpiuser/.ssh/id_rsa node$i:/home/mpiuser/.ssh/id_rsa
    docker cp node1:/home/mpiuser/.ssh/id_rsa.pub node$i:/home/mpiuser/.ssh/id_rsa.pub
    docker cp node1:/home/mpiuser/.ssh/authorized_keys node$i:/home/mpiuser/.ssh/authorized_keys
    docker cp node1:/home/mpiuser/.ssh/config node$i:/home/mpiuser/.ssh/config
    docker exec node$i chown -R mpiuser:mpiuser /home/mpiuser/.ssh
done

for i in $(seq 1 $MACHINE_COUNT)
do
    docker cp $PROGRAM_PATH node$i:/home/mpiuser/
    docker exec node$i chown -R mpiuser:mpiuser /home/mpiuser/$(basename $PROGRAM_PATH)
    docker exec -u mpiuser node$i sh -c "cd /home/mpiuser/$(basename $PROGRAM_PATH) && mkdir -p build && cd build && cmake .. && make"
done

docker exec -u mpiuser node1 sh -c "mpirun --mca pml ob1 --mca btl tcp,self --mca mtl ^psm,^psm2 --mca plm_rsh_args '-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null' -np $MACHINE_COUNT -H $(for i in $(seq 1 $MACHINE_COUNT); do echo -n node$i,; done | sed 's/,$//') /home/mpiuser/$(basename $PROGRAM_PATH)/build/$(basename $PROGRAM_PATH) $PROGRAM_ARGS"

rm Dockerfile
