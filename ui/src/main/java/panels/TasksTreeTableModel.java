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
        boolean parentChanged = false;
        TaskTreeTableNode node = findTaskNode(parentTask, task.id);

        if (node != null) {
            node.setUserObject(task);

            TreeTableNode currentParent = node.getParent();

            // this is the root task on a specific task list and we don't show the root, skip it
            if (currentParent == null) {
                // TODO maybe there's an issue here if we finish the task?
                return;
            }
            TreePath parentPath = new TreePath(getPathToRoot(currentParent));
            int index = currentParent.getIndex(node);

            if (task.state == TaskState.FINISHED || parentChanged) {
                node.removeFromParent();
                modelSupport.fireChildRemoved(parentPath, index, node);
            }
            else {
                // TODO exception here when updating task to a parent that's finished (i.e. hidden)?
                modelSupport.fireChildChanged(parentPath, index, node);
            }

            if (parentChanged) {
                TaskTreeTableNode parent = findTaskNode(parentTask, task.parentID);
                parent.add(node);

                modelSupport.fireChildAdded(new TreePath(getPathToRoot(parent)), parent.getIndex(node), node);
            }
        }
        else {
            addTask(task);
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
