package panels;

import data.Task;
import org.jdesktop.swingx.treetable.AbstractTreeTableModel;
import org.jdesktop.swingx.treetable.TreeTableNode;

import javax.swing.tree.TreeNode;
import javax.swing.tree.TreePath;

public class TasksTreeTableModel extends AbstractTreeTableModel {
    private final ParentTaskTreeTableNode parentTask;

    public TasksTreeTableModel() {
        super(new ParentTaskTreeTableNode());

        parentTask = (ParentTaskTreeTableNode) getRoot();
    }

    public void addTask(Task task) {
        // find the parent task first, then add the node to that
        if (task.parentID == 0) {
            TaskTreeTableNode child = new TaskTreeTableNode(parentTask, task);
            parentTask.add(child);

            modelSupport.fireChildAdded(new TreePath(parentTask), parentTask.getIndex(child), child);
        }
        else {
            TaskTreeTableNode node = findTaskNode(parentTask, task.parentID);

            if (node != null) {
                node.add(new TaskTreeTableNode(node, task));

                modelSupport.fireChildAdded(new TreePath(node.getParent()), node.getParent().getIndex(node), node);
            }
        }
    }

    public void updateTask(Task task) {

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
        return null;
    }

    @Override
    public int getColumnCount() {
        return 2;
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
}
