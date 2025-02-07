package panels;

import data.Task;
import data.TaskState;
import org.jdesktop.swingx.treetable.AbstractTreeTableModel;
import org.jdesktop.swingx.treetable.TreeTableNode;

import javax.swing.tree.TreeNode;
import javax.swing.tree.TreePath;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

public class TasksTreeTableModel extends AbstractTreeTableModel {
    private TaskTreeTableNode parentTask;

    public TasksTreeTableModel(TaskTreeTableNode root) {
        super(root);

        parentTask = root;
    }

    public TreeTableNode[] getPathToRoot(TreeTableNode aNode) {
        if (aNode == null) {
            TreeTableNode[] treeTableNodes = new TreeTableNode[1];
            treeTableNodes[0] = parentTask;
            return treeTableNodes;
        }
        Objects.requireNonNull(aNode);

        List<TreeTableNode> path = new ArrayList<>();

        TreeTableNode node;
        for(node = aNode; node != this.root; node = node.getParent()) {
            path.addFirst(node);
        }

        path.addFirst(node);

        return path.toArray(new TreeTableNode[0]);
    }

    public void addTask(Task task) {
        // don't add finished tasks for now. maybe eventually we'll use some sort of filter to hide them instead of removing the nodes.
        if (task.state == TaskState.FINISHED) {
            return;
        }

        // find the parent task first, then add the node to that
//        if (task.parentID == 0) {
//            TaskTreeTableNode child = new TaskTreeTableNode(parentTask, task);
//            parentTask.add(child);
//
//            modelSupport.fireChildAdded(new TreePath(parentTask), parentTask.getIndex(child), child);
//        }
//        else {
//
//        }
        TaskTreeTableNode parent = findTaskNode(parentTask, task.parentID);

        if (parent != null) {
            TaskTreeTableNode childNode = new TaskTreeTableNode(parent, task);
            parent.add(childNode);

            modelSupport.fireChildAdded(new TreePath(getPathToRoot(parent)), parent.getIndex(childNode), childNode);

            for (Task child : task.children) {
                addTask(child);
            }
        }
    }

    public void updateTask(Task task) {
        TaskTreeTableNode node = findTaskNode(parentTask, task.id);

        if (node != null) {
            node.setUserObject(task);

            TreePath parentPath = new TreePath(getPathToRoot(node.getParent()));
            int index = node.getParent().getIndex(node);

            if (task.state == TaskState.FINISHED) {
                node.removeFromParent();
                modelSupport.fireChildRemoved(parentPath, index, node);
            }
            modelSupport.fireChildChanged(parentPath, index, node);
        }
    }

    private static TaskTreeTableNode findTaskNode(TaskTreeTableNode node, int taskID) {
        for (int i = 0; i < node.getChildCount(); i++) {
            TreeTableNode child = node.getChildAt(i);

            if (((Task) child.getUserObject()).id == taskID) {
                return (TaskTreeTableNode) child;
            }
            else {
                TaskTreeTableNode next = findTaskNode((TaskTreeTableNode) child, taskID);

                if (next != null) {
                    return next;
                }
            }
        }
        if (node.getTaskID() == taskID) {
            return node;
        }
        return null;
    }

    @Override
    public int getColumnCount() {
        return 1;
    }

    @Override
    public Object getValueAt(Object node, int column) {
        return ((TaskTreeTableNode) node).getValueAt(column);
    }

    @Override
    public Object getChild(Object parent, int index) {
        /*
        exception when finishing a child of a child
        Exception in thread "AWT-EventQueue-0" java.lang.IndexOutOfBoundsException: Index 1 out of bounds for length 1
	at java.base/jdk.internal.util.Preconditions.outOfBounds(Preconditions.java:100)
	at java.base/jdk.internal.util.Preconditions.outOfBoundsCheckIndex(Preconditions.java:106)
	at java.base/jdk.internal.util.Preconditions.checkIndex(Preconditions.java:302)
	at java.base/java.util.Objects.checkIndex(Objects.java:385)
	at java.base/java.util.ArrayList.get(ArrayList.java:427)
	at org.jdesktop.swingx.treetable.AbstractMutableTreeTableNode.getChildAt(AbstractMutableTreeTableNode.java:166)
	at panels.TasksTreeTableModel.getChild(TasksTreeTableModel.java:123)
	at java.desktop/javax.swing.plaf.basic.BasicTreeUI$Handler.treeNodesChanged(BasicTreeUI.java:4329)
	at org.jdesktop.swingx.tree.TreeModelSupport.fireChildrenChanged(TreeModelSupport.java:192)
	at org.jdesktop.swingx.tree.TreeModelSupport.fireChildChanged(TreeModelSupport.java:158)
	at panels.TasksTreeTableModel.updateTask(TasksTreeTableModel.java:86)
	at panels.TasksLists.updatedTask(TasksLists.java:330)
	at data.TaskModel.lambda$receiveInfo$12(TaskModel.java:75)
	at java.base/java.util.ArrayList.forEach(ArrayList.java:1596)
	at data.TaskModel.receiveInfo(TaskModel.java:75)
	at data.ServerConnection.lambda$run$0(ServerConnection.java:56)
	at java.desktop/java.awt.event.InvocationEvent.dispatch(InvocationEvent.java:318)
	at java.desktop/java.awt.EventQueue.dispatchEventImpl(EventQueue.java:773)
	at java.desktop/java.awt.EventQueue$4.run(EventQueue.java:720)
	at java.desktop/java.awt.EventQueue$4.run(EventQueue.java:714)
	at java.base/java.security.AccessController.doPrivileged(AccessController.java:400)
	at java.base/java.security.ProtectionDomain$JavaSecurityAccessImpl.doIntersectionPrivilege(ProtectionDomain.java:87)
	at java.desktop/java.awt.EventQueue.dispatchEvent(EventQueue.java:742)
	at java.desktop/java.awt.EventDispatchThread.pumpOneEventForFilters(EventDispatchThread.java:203)
	at java.desktop/java.awt.EventDispatchThread.pumpEventsForFilter(EventDispatchThread.java:124)
	at java.desktop/java.awt.EventDispatchThread.pumpEventsForHierarchy(EventDispatchThread.java:113)
	at java.desktop/java.awt.EventDispatchThread.pumpEvents(EventDispatchThread.java:109)
	at java.desktop/java.awt.EventDispatchThread.pumpEvents(EventDispatchThread.java:101)
	at java.desktop/java.awt.EventDispatchThread.run(EventDispatchThread.java:90)
         */
        return ((TaskTreeTableNode) parent).getChildAt(index);
    }

    @Override
    public int getChildCount(Object parent) {
        return ((TaskTreeTableNode) parent).getChildCount();
    }

    @Override
    public int getIndexOfChild(Object parent, Object child) {
        return ((TaskTreeTableNode) parent).getIndex((TreeNode) child);
    }

    public void setRoot(TaskTreeTableNode root) {
        this.root = root;
        parentTask = root;
        modelSupport.fireNewRoot();
    }
}
